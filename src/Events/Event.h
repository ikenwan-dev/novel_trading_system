#pragma once

// Enum for all event subtypes. This will be used for easy and readible checking
// of event types
enum class EventType { Market, Signal, Order, Fill };

// Base class for all event sub types. Sub types should have a corresponding
// enum EventType
class Event {
public:
  virtual ~Event() = default;

  // Pure virtual function to get the event type.
  // MUST BE IMPLEMENTED BY SUBCLASSES
  virtual EventType get_type() const = 0;
};