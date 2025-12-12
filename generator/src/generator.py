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
        output_file: Path,
        all_includes: List[str] = None
    ) -> None:
        """
        Generate a .proto file for a single MAVLink dialect.

        Args:
            dialect: Parsed MAVLink dialect
            output_file: Output .proto file path
            all_includes: All transitive includes (optional)
        """
        # Load template
        template = self.env.get_template("dialect.proto.j2")

        # Use all_includes if provided, otherwise use dialect.includes
        includes_to_use = all_includes if all_includes is not None else dialect.includes

        # Render template
        content = template.render(
            dialect=dialect,
            all_includes=includes_to_use
        )

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
            dialects: List of parsed MAVLink dialects
            output_dir: Output directory for .proto files
        """
        dialect_dict = {}

        # Build enum lookup table (enum_name -> dialect_name)
        enum_to_dialect = {}
        for dialect in dialects:
            for enum_name in dialect.enums.keys():
                if enum_name not in enum_to_dialect:
                    enum_to_dialect[enum_name] = dialect.name

        # Register enum lookup helper for templates
        def resolve_enum_package(enum_name: str, current_dialect_name: str) -> str:
            """
            Resolve enum reference with package prefix if from another dialect.

            Args:
                enum_name: The enum name (e.g., "MAV_TYPE")
                current_dialect_name: Current dialect being generated

            Returns:
                Qualified enum name (e.g., "minimal.MavType" or "MavType")
            """
            dialect_name = enum_to_dialect.get(enum_name)
            if dialect_name and dialect_name != current_dialect_name:
                # Enum is from another dialect, add package prefix
                sanitized_enum = TypeConverter.sanitize_enum_name(enum_name)
                return f"{dialect_name}.{sanitized_enum}"
            else:
                # Enum is in current dialect or not found (will fail later)
                return TypeConverter.sanitize_enum_name(enum_name)

        # Update template globals
        self.env.globals["resolve_enum_package"] = resolve_enum_package

        # Generate individual dialect protos
        for dialect in dialects:
            proto_file = output_dir / "mavlink" / f"{dialect.name}.proto"

            # Pass current dialect name to template
            self.env.globals["current_dialect_name"] = dialect.name

            # Get all transitive includes if parser is available
            all_includes = None
            if self.parser:
                all_includes = self.parser.get_all_includes(dialect.name)

            self.generate_dialect_proto(dialect, proto_file, all_includes)
            dialect_dict[dialect.name] = dialect

        # Generate bridge service proto
        bridge_file = output_dir / "mavlink_bridge.proto"
        self.generate_bridge_service(dialect_dict, bridge_file)

        print(f"\n✓ Generated {len(dialects)} dialect proto(s) + 1 bridge service")
        print(f"  Output directory: {output_dir}")
