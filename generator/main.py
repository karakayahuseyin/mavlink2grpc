#!/usr/bin/env python3
"""
MAVLink to Protocol Buffer Generator - Interactive CLI
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from src.parser import MAVLinkParser
from src.generator import ProtoGenerator
from rich.console import Console
from rich.panel import Panel
from rich.prompt import Prompt
from rich.table import Table

console = Console()


def main():
    # Header
    console.print(Panel.fit(
        "[bold cyan]MAVLink to Protocol Buffer Generator[/bold cyan]",
        border_style="cyan"
    ))
    console.print()

    # Ask user which dialect to generate
    console.print("[bold]Which dialect would you like to generate?[/bold]\n")

    table = Table(show_header=False, box=None, padding=(0, 2))
    table.add_column("Choice", style="cyan bold")
    table.add_column("Dialect")
    table.add_column("Info", style="dim")

    table.add_row("1", "Minimal", "(6 enums, 1 message)")
    table.add_row("2", "Standard", "(includes minimal)")
    table.add_row("3", "Common", "(includes standard + minimal)")

    console.print(table)
    console.print()

    # Get user choice
    choice = Prompt.ask(
        "Your choice",
        choices=["1", "2", "3"],
        default="3"
    )

    # Map choice to dialect
    dialect_map = {
        "1": "minimal",
        "2": "standard",
        "3": "common",
    }
    selected_dialect = dialect_map[choice]

    console.print(f"\n[cyan]Selected dialect:[/cyan] {selected_dialect}\n")

    # Setup paths
    xml_dir = Path(__file__).parent.parent / "mavlink" / "message_definitions" / "v1.0"
    output_dir = Path(__file__).parent.parent / "proto"
    template_dir = Path(__file__).parent / "templates"

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    # Parse dialect (with includes merged)
    console.print("[bold]Parsing dialect (including dependencies)...[/bold]")
    mavlink_parser = MAVLinkParser(xml_dir)

    try:
        dialect = mavlink_parser.get_flattened_dialect(selected_dialect)
        console.print(f"  [green]✓[/green] {selected_dialect}: {len(dialect.enums)} enums, {len(dialect.messages)} messages")
    except Exception as e:
        console.print(f"  [red]✗[/red] {selected_dialect}: ERROR - {e}")
        return 1

    # Generate proto file
    console.print(f"\n[bold]Generating proto file...[/bold]")
    proto_gen = ProtoGenerator(template_dir, mavlink_parser)

    try:
        proto_gen.generate_all([dialect], output_dir)
        console.print(f"  [green]✓[/green] Generated {selected_dialect}.proto")
        console.print(f"  [green]✓[/green] Generated bridge service proto")
    except Exception as e:
        console.print(f"  [red]✗[/red] ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1

    # Summary
    total_enums = len(dialect.enums)
    total_messages = len(dialect.messages)

    console.print()
    console.print(Panel.fit(
        f"[bold green]Successfully completed![/bold green]\n\n"
        f"Output directory: [cyan]{output_dir}[/cyan]\n"
        f"Total: [yellow]{total_enums}[/yellow] enums, [yellow]{total_messages}[/yellow] messages",
        border_style="green"
    ))

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        console.print("\n\n[yellow]Operation cancelled.[/yellow]")
        sys.exit(1)
