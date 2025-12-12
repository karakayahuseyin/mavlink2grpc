"""
MAVLink XML parser.
"""
from pathlib import Path
from typing import Dict, Set
from lxml import etree

from .models import (
    MAVLinkDialect,
    Enum,
    EnumEntry,
    Message,
    MessageField,
)


class MAVLinkParser:
    """Parser for MAVLink XML definition files."""

    def __init__(self, xml_dir: Path):
        """
        Initialize parser.

        Args:
            xml_dir: Directory containing MAVLink XML files
        """
        self.xml_dir = Path(xml_dir)
        self.parsed_dialects: Dict[str, MAVLinkDialect] = {}

    def parse_file(self, xml_file: str) -> MAVLinkDialect:
        """
        Parse a single MAVLink XML file.

        Args:
            xml_file: Name of the XML file (e.g., "common.xml")

        Returns:
            Parsed MAVLinkDialect object
        """
        # Check if already parsed (for includes)
        dialect_name = xml_file.replace('.xml', '')
        if dialect_name in self.parsed_dialects:
            return self.parsed_dialects[dialect_name]

        file_path = self.xml_dir / xml_file
        if not file_path.exists():
            raise FileNotFoundError(f"MAVLink XML file not found: {file_path}")

        # Parse XML
        tree = etree.parse(str(file_path))
        root = tree.getroot()

        # Create dialect object
        dialect = MAVLinkDialect(
            name=dialect_name,
            file_path=str(file_path),
        )

        # Parse top-level attributes
        version_elem = root.find('version')
        if version_elem is not None and version_elem.text:
            dialect.version = int(version_elem.text)

        dialect_elem = root.find('dialect')
        if dialect_elem is not None and dialect_elem.text:
            dialect.dialect = int(dialect_elem.text)

        # Parse includes
        for include_elem in root.findall('include'):
            if include_elem.text:
                dialect.includes.append(include_elem.text)

        # Parse enums
        enums_elem = root.find('enums')
        if enums_elem is not None:
            for enum_elem in enums_elem.findall('enum'):
                enum = self._parse_enum(enum_elem)
                dialect.enums[enum.name] = enum

        # Parse messages
        messages_elem = root.find('messages')
        if messages_elem is not None:
            for message_elem in messages_elem.findall('message'):
                message = self._parse_message(message_elem)
                dialect.messages[message.id] = message

        # Cache the parsed dialect
        self.parsed_dialects[dialect_name] = dialect

        # Recursively parse includes
        for include_file in dialect.includes:
            self.parse_file(include_file)

        return dialect

    def _parse_enum(self, elem: etree._Element) -> Enum:
        """Parse an <enum> element."""
        name = elem.get('name', '')
        is_bitmask = elem.get('bitmask') == 'true'

        # Parse description
        desc_elem = elem.find('description')
        description = desc_elem.text.strip() if desc_elem is not None and desc_elem.text else None

        # Parse deprecated info
        deprecated_elem = elem.find('deprecated')
        deprecated = None
        replaced_by = None
        if deprecated_elem is not None:
            deprecated = deprecated_elem.get('since')
            replaced_by = deprecated_elem.get('replaced_by')

        # Parse entries
        entries = []
        for entry_elem in elem.findall('entry'):
            entry = self._parse_enum_entry(entry_elem)
            entries.append(entry)

        return Enum(
            name=name,
            description=description,
            is_bitmask=is_bitmask,
            entries=entries,
            deprecated=deprecated,
            replaced_by=replaced_by,
        )

    def _parse_enum_entry(self, elem: etree._Element) -> EnumEntry:
        """Parse an <entry> element within an enum."""
        value = int(elem.get('value', '0'))
        name = elem.get('name', '')

        # Parse description
        desc_elem = elem.find('description')
        description = desc_elem.text.strip() if desc_elem is not None and desc_elem.text else None

        # Parse deprecated info
        deprecated_elem = elem.find('deprecated')
        deprecated = None
        replaced_by = None
        if deprecated_elem is not None:
            deprecated = deprecated_elem.get('since')
            replaced_by = deprecated_elem.get('replaced_by')

        return EnumEntry(
            value=value,
            name=name,
            description=description,
            deprecated=deprecated,
            replaced_by=replaced_by,
        )

    def _parse_message(self, elem: etree._Element) -> Message:
        """Parse a <message> element."""
        msg_id = int(elem.get('id', '0'))
        name = elem.get('name', '')

        # Parse description
        desc_elem = elem.find('description')
        description = None
        if desc_elem is not None and desc_elem.text:
            # Clean up multiline descriptions
            description = ' '.join(desc_elem.text.strip().split())

        # Parse deprecated/superseded info
        deprecated_elem = elem.find('deprecated')
        deprecated = deprecated_elem.get('since') if deprecated_elem is not None else None
        replaced_by = deprecated_elem.get('replaced_by') if deprecated_elem is not None else None

        superseded_elem = elem.find('superseded')
        superseded = superseded_elem.get('since') if superseded_elem is not None else None
        if superseded and superseded_elem is not None:
            replaced_by = superseded_elem.get('replaced_by')

        # Check if work-in-progress
        wip = elem.find('wip') is not None

        # Parse fields
        fields = []
        for field_elem in elem.findall('field'):
            field = self._parse_field(field_elem)
            fields.append(field)

        return Message(
            id=msg_id,
            name=name,
            description=description,
            fields=fields,
            deprecated=deprecated,
            superseded=superseded,
            replaced_by=replaced_by,
            wip=wip,
        )

    def _parse_field(self, elem: etree._Element) -> MessageField:
        """Parse a <field> element within a message."""
        field_type = elem.get('type', '')
        name = elem.get('name', '')
        enum = elem.get('enum')
        units = elem.get('units')
        invalid = elem.get('invalid')
        print_format = elem.get('print_format')

        # Parse description
        description = elem.text.strip() if elem.text else None

        return MessageField(
            type=field_type,
            name=name,
            enum=enum,
            units=units,
            description=description,
            invalid=invalid,
            print_format=print_format,
        )

    def merge_dialects(self, base_dialect: str, *include_dialects: str) -> MAVLinkDialect:
        """
        Merge multiple dialects into one (resolving includes).

        Args:
            base_dialect: The main dialect name (e.g., "common")
            *include_dialects: Additional dialects to merge

        Returns:
            Merged MAVLinkDialect
        """
        # Parse all dialects
        dialects_to_merge = [base_dialect] + list(include_dialects)
        for dialect_name in dialects_to_merge:
            if dialect_name not in self.parsed_dialects:
                self.parse_file(f"{dialect_name}.xml")

        # Start with base
        merged = MAVLinkDialect(
            name=base_dialect,
            file_path=self.parsed_dialects[base_dialect].file_path,
            version=self.parsed_dialects[base_dialect].version,
            dialect=self.parsed_dialects[base_dialect].dialect,
        )

        # Track what we've already added to avoid duplicates
        seen_enums: Set[str] = set()
        seen_messages: Set[int] = set()

        # Merge in dependency order (includes first, then base)
        for dialect_name in dialects_to_merge:
            dialect = self.parsed_dialects[dialect_name]

            # Merge enums
            for enum_name, enum in dialect.enums.items():
                if enum_name not in seen_enums:
                    merged.enums[enum_name] = enum
                    seen_enums.add(enum_name)

            # Merge messages
            for msg_id, message in dialect.messages.items():
                if msg_id not in seen_messages:
                    merged.messages[msg_id] = message
                    seen_messages.add(msg_id)

        return merged

    def resolve_includes(self, dialect_name: str) -> MAVLinkDialect:
        """
        Parse a dialect and all its includes, returning merged result.

        Args:
            dialect_name: Name of the dialect (without .xml)

        Returns:
            Fully resolved MAVLinkDialect
        """
        # Parse the main file
        dialect = self.parse_file(f"{dialect_name}.xml")

        # If no includes, return as-is
        if not dialect.includes:
            return dialect

        # Recursively resolve includes
        include_names = [inc.replace('.xml', '') for inc in dialect.includes]
        return self.merge_dialects(dialect_name, *include_names)
