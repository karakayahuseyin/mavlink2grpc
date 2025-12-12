#!/usr/bin/env python3
"""
Test script for proto generation.
Tests with minimal.xml for a clean, simple output.
"""
import sys
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from src.parser import MAVLinkParser
from src.generator import ProtoGenerator
from rich.console import Console
from rich.panel import Panel

console = Console()


def main():
    """Test proto generation with minimal.xml."""
    console.print(Panel.fit("🔨 Proto Generator Test (minimal.xml)", style="bold blue"))

    # Paths
    xml_dir = Path(__file__).parent.parent.parent / "mavlink" / "message_definitions" / "v1.0"
    template_dir = Path(__file__).parent.parent / "templates"
    output_dir = Path(__file__).parent.parent.parent / "proto"

    if not xml_dir.exists():
        console.print(f"[red]Error: MAVLink definitions not found at {xml_dir}[/red]")
        return

    console.print(f"XML dir: {xml_dir}")
    console.print(f"Template dir: {template_dir}")
    console.print(f"Output dir: {output_dir}\n")

    # Parse minimal.xml
    console.print("[cyan]Step 1: Parsing minimal.xml...[/cyan]")
    parser = MAVLinkParser(xml_dir)
    minimal = parser.parse_file("minimal.xml")

    console.print(f"✓ Parsed minimal.xml")
    console.print(f"  - Enums: {len(minimal.enums)}")
    console.print(f"  - Messages: {len(minimal.messages)}")

    # Show what we're generating
    console.print("\n[cyan]Step 2: Generating proto files...[/cyan]")

    # Create generator
    generator = ProtoGenerator(template_dir)

    # Generate protos
    generator.generate_all([minimal], output_dir)

    # Show generated files
    console.print("\n[green]Generated files:[/green]")
    proto_files = sorted(output_dir.rglob("*.proto"))
    for proto_file in proto_files:
        rel_path = proto_file.relative_to(output_dir)
        size = proto_file.stat().st_size
        console.print(f"  📄 {rel_path} ({size} bytes)")

    # Show preview of minimal.proto
    minimal_proto = output_dir / "mavlink" / "minimal.proto"
    if minimal_proto.exists():
        console.print("\n[cyan]Preview of minimal.proto (first 30 lines):[/cyan]")
        with open(minimal_proto) as f:
            lines = f.readlines()
            for i, line in enumerate(lines[:30], 1):
                console.print(f"{i:3d} │ {line}", end="")
            if len(lines) > 30:
                console.print(f"... ({len(lines) - 30} more lines)")

    console.print("\n[bold green]✓ Proto generation successful![/bold green]")
    console.print(f"\nNext step: Validate with protoc")
    console.print(f"  cd {output_dir.parent}")
    console.print(f"  protoc --proto_path=proto --cpp_out=. proto/mavlink/minimal.proto")


if __name__ == "__main__":
    main()
