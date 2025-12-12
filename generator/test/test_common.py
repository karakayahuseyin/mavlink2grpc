#!/usr/bin/env python3
"""
Test proto generation with common.xml (full MAVLink protocol).
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.parser import MAVLinkParser
from src.generator import ProtoGenerator
from rich.console import Console
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, TextColumn

console = Console()


def main():
    """Test proto generation with common.xml."""
    console.print(Panel.fit("🔨 Proto Generator Test (common.xml)", style="bold blue"))

    # Paths
    xml_dir = Path(__file__).parent.parent.parent / "mavlink" / "message_definitions" / "v1.0"
    template_dir = Path(__file__).parent.parent / "templates"
    output_dir = Path(__file__).parent.parent.parent / "proto"

    console.print(f"Output dir: {output_dir}\n")

    # Parse dialects separately (not merged)
    console.print("[cyan]Step 1: Parsing dialects...[/cyan]")

    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console,
    ) as progress:
        task = progress.add_task("Parsing XML files...", total=None)

        parser = MAVLinkParser(xml_dir)

        # Parse each dialect separately (maintains include structure)
        minimal = parser.parse_file("minimal.xml")
        standard = parser.parse_file("standard.xml")
        common = parser.parse_file("common.xml")

        progress.update(task, completed=True)

    console.print(f"✓ Parsed 3 dialects")
    console.print(f"  - minimal: {len(minimal.enums)} enums, {len(minimal.messages)} messages")
    console.print(f"  - standard: {len(standard.enums)} enums, {len(standard.messages)} messages")
    console.print(f"  - common: {len(common.enums)} enums, {len(common.messages)} messages")

    # Generate protos
    console.print("\n[cyan]Step 2: Generating proto files...[/cyan]")

    # Pass parser to generator for transitive include resolution
    generator = ProtoGenerator(template_dir, parser=parser)

    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console,
    ) as progress:
        task = progress.add_task("Generating protos...", total=None)

        # Generate all dialects
        generator.generate_all([minimal, standard, common], output_dir)

        progress.update(task, completed=True)

    # Show generated files
    console.print("\n[green]Generated files:[/green]")
    proto_files = sorted(output_dir.rglob("*.proto"))
    for proto_file in proto_files:
        rel_path = proto_file.relative_to(output_dir)
        size = proto_file.stat().st_size
        lines = len(proto_file.read_text().splitlines())
        console.print(f"  📄 {rel_path} ({size:,} bytes, {lines:,} lines)")

    # Show some message examples
    console.print("\n[cyan]Sample messages in common.proto:[/cyan]")

    # Show a few interesting messages
    sample_messages = [
        (0, "HEARTBEAT"),
        (1, "SYS_STATUS"),
        (33, "GLOBAL_POSITION_INT"),
        (30, "ATTITUDE"),
        (76, "COMMAND_LONG"),
    ]

    for msg_id in sample_messages:
        if isinstance(msg_id, tuple):
            msg_id, expected_name = msg_id
        else:
            expected_name = None

        if msg_id in common.messages:
            msg = common.messages[msg_id]
            console.print(f"  • [{msg_id:4d}] {msg.name:30s} - {len(msg.fields)} fields")

    console.print("\n[bold green]✓ Proto generation successful![/bold green]")
    console.print(f"\nNext: Validate with protoc")


if __name__ == "__main__":
    main()
