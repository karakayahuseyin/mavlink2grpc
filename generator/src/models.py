"""
Data models for MAVLink XML structures.
"""
from typing import Optional, List, Dict
from pydantic import BaseModel, Field


class EnumEntry(BaseModel):
    """Represents a single enum entry."""
    value: int
    name: str
    description: Optional[str] = None
    deprecated: Optional[str] = None
    replaced_by: Optional[str] = None


class Enum(BaseModel):
    """Represents a MAVLink enum."""
    name: str
    description: Optional[str] = None
    is_bitmask: bool = False
    entries: List[EnumEntry] = Field(default_factory=list)
    deprecated: Optional[str] = None
    replaced_by: Optional[str] = None


class MessageField(BaseModel):
    """Represents a field in a MAVLink message."""
    type: str  # e.g., "uint8_t", "float", "char[16]"
    name: str
    enum: Optional[str] = None  # Reference to enum name
    units: Optional[str] = None  # e.g., "m/s", "rad"
    description: Optional[str] = None
    invalid: Optional[str] = None  # Invalid value indicator
    print_format: Optional[str] = None

    # Computed properties
    @property
    def is_array(self) -> bool:
        """Check if field is an array type (e.g., char[16], uint8_t[8])."""
        return '[' in self.type and ']' in self.type

    @property
    def array_length(self) -> Optional[int]:
        """Extract array length if this is an array field."""
        if not self.is_array:
            return None
        start = self.type.index('[')
        end = self.type.index(']')
        return int(self.type[start + 1:end])

    @property
    def base_type(self) -> str:
        """Get the base type without array notation."""
        if self.is_array:
            return self.type[:self.type.index('[')]
        return self.type


class Message(BaseModel):
    """Represents a MAVLink message."""
    id: int
    name: str
    description: Optional[str] = None
    fields: List[MessageField] = Field(default_factory=list)
    deprecated: Optional[str] = None
    superseded: Optional[str] = None
    replaced_by: Optional[str] = None
    wip: bool = False  # Work in progress flag


class MAVLinkDialect(BaseModel):
    """Represents a complete MAVLink dialect (XML file)."""
    name: str  # e.g., "minimal", "common", "ardupilotmega"
    file_path: str
    version: Optional[int] = None
    dialect: Optional[int] = None
    includes: List[str] = Field(default_factory=list)  # Other XML files to include
    enums: Dict[str, Enum] = Field(default_factory=dict)
    messages: Dict[int, Message] = Field(default_factory=dict)

    def get_message_by_name(self, name: str) -> Optional[Message]:
        """Find message by name."""
        for msg in self.messages.values():
            if msg.name == name:
                return msg
        return None

    def get_enum_by_name(self, name: str) -> Optional[Enum]:
        """Find enum by name."""
        return self.enums.get(name)
