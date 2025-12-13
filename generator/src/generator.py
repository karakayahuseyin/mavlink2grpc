"""
Protocol Buffer (.proto) file generator.
"""
from pathlib import Path
from typing import Dict, List
from jinja2 import Environment, FileSystemLoader

from .models import MAVLinkDialect
from .type_converter import TypeConverter


class ProtoGenerator:
    """Generates .proto files from parsed MAVLink dialects."""

    def __init__(self, template_dir: Path, parser=None):
        """
        Initialize generator with Jinja2 templates.

        Args:
            template_dir: Directory containing .proto.j2 templates
            parser: Optional MAVLinkParser for resolving includes
        """
        self.env = Environment(
            loader=FileSystemLoader(str(template_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
        )

        # Register custom functions for templates
        self.env.globals.update({
            "sanitize_enum_name": TypeConverter.sanitize_enum_name,
            "sanitize_message_name": TypeConverter.sanitize_message_name,
            "sanitize_field_name": TypeConverter.sanitize_field_name,
            "to_proto_type": TypeConverter.to_proto_type,
            "format_comment": TypeConverter.format_comment,
        })

        self.parser = parser

    def generate_dialect_proto(
        self,
        dialect: MAVLinkDialect,
        output_file: Path
    ) -> None:
        """
        Generate a .proto file for a single MAVLink dialect.

        Args:
            dialect: Parsed MAVLink dialect (should be flattened with includes merged)
            output_file: Output .proto file path
        """
        # Load template
        template = self.env.get_template("dialect.proto.j2")

        # Render template
        content = template.render(dialect=dialect)

        # Ensure output directory exists
        output_file.parent.mkdir(parents=True, exist_ok=True)

        # Write file
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(content)

        print(f"✓ Generated {output_file}")

    def generate_bridge_service(
        self,
        dialects: Dict[str, MAVLinkDialect],
        output_file: Path
    ) -> None:
        """
        Generate the bridge service .proto file.

        Args:
            dialects: Dictionary of dialect_name -> MAVLinkDialect
            output_file: Output .proto file path
        """
        # Load template
        template = self.env.get_template("bridge_service.proto.j2")

        # Render template
        content = template.render(
            dialect_names=sorted(dialects.keys()),
            dialects=dialects,
            sanitize_message_name=TypeConverter.sanitize_message_name,
        )

        # Ensure output directory exists
        output_file.parent.mkdir(parents=True, exist_ok=True)

        # Write file
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(content)

        print(f"✓ Generated {output_file}")

    def generate_all(
        self,
        dialects: List[MAVLinkDialect],
        output_dir: Path
    ) -> None:
        """
        Generate all .proto files for given dialects.

        Args:
            dialects: List of parsed MAVLink dialects (should be flattened)
            output_dir: Output directory for .proto files
        """
        dialect_dict = {}

        # Simple enum resolution - no cross-package references needed
        # All enums are in the same package now (flattened)
        def resolve_enum_package(enum_name: str, current_dialect_name: str) -> str:
            """
            Resolve enum reference - just sanitize the name.
            No cross-package resolution needed since dialects are flattened.
            """
            return TypeConverter.sanitize_enum_name(enum_name)

        # Update template globals
        self.env.globals["resolve_enum_package"] = resolve_enum_package

        # Generate individual dialect protos
        for dialect in dialects:
            proto_file = output_dir / "mavlink" / f"{dialect.name}.proto"
            self.generate_dialect_proto(dialect, proto_file)
            dialect_dict[dialect.name] = dialect

        # Generate bridge service proto
        bridge_file = output_dir / "mavlink_bridge.proto"
        self.generate_bridge_service(dialect_dict, bridge_file)

        print(f"\n✓ Generated {len(dialects)} dialect proto(s) + 1 bridge service")
        print(f"  Output directory: {output_dir}")
