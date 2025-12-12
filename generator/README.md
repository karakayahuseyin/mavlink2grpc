# Generator

Code generator that converts MAVLink XML definitions to Protocol Buffer (.proto) files. Written in Python using Pydantic for data modeling, Rich and Typer for CLI and Jinja2 for templating.

## Features

### Currently implemented:

* Parses MAVLink XML schema files (enums, messages, fields)

### Work in progress:

* Resolves include dependencies between dialect files
* Generates type-safe Protocol Buffer definitions
* Preserves metadata (descriptions, units, deprecation info)

## Structure

```text
generator/
├── src/              # Source code
│   ├── models.py     # Pydantic data models
│   ├── parser.py     # XML parser
│   └── generator.py  # Proto file generator (WIP)
├── test/             # Tests
│   └── test_parser.py
└── requirements.txt  # Python dependencies
```

## Usage

```bash
# Install dependencies
pip install -r requirements.txt

# Run parser test
python test/test_parser.py
```
