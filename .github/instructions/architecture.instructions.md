# Project Architecture and Coding Standards

- **Principles**: Adhere strictly to SOLID and DRY principles.
- **Architecture**: Domain-Driven Design (DDD) organized by bounded contexts.
- **Project Structure**: Use idiomatic Go organization - group by domain/feature, not by layer.
- **Domain Packages**: Organize code under `internal/` by domain (sales, inventory, build, etc.). Each domain package contains its handlers, services, and types together.
- **Data Layer**: Responsible only for database operations.
- **Service Layer**: Contains all business logic. Should be decoupled from specific UI frameworks.
- **Tech Stack**: GO, Vue 3, GraphQL, JSON.

## Domain-Driven Design

- **Bounded Contexts**: Each domain (sales, inventory, build) represents a bounded context with clear boundaries.
- **Package Organization**: Organize by domain, not by technical layer. Follow Go idioms by grouping related functionality together.
- **Domain Packages**: Place handlers, services, and domain types in the same package (e.g., `internal/sales/`).
- **Aggregates**: Keep domain entities and their business logic together within their domain package.
- **Shared Code**: Common utilities and cross-cutting concerns go in `internal/common/` or similar shared packages.

## DRY (Don't Repeat Yourself):

- "Prioritize reusability. If you identify logic that can be abstracted into a shared utility or base class, suggest that refactor instead of duplicating code".
- "Ensure all configuration values and magic strings are centralized in dedicated constant or config files".

## SOLID Principles:

- Single Responsibility: "Each class and function must have a single, well-defined purpose. Break down complex logic into smaller, testable units".
- Open/Closed: "Design for extension. Prefer interfaces and abstract classes over hardcoded logic to allow for new features without modifying existing code".
- Liskov Substitution: "Ensure subclasses can replace their base classes without altering the correctness of the program."
- Interface Segregation: "Avoid bloated interfaces; favor small, client-specific interfaces".
- Dependency Inversion: "Always depend on abstractions (interfaces) rather than concrete implementations. Use dependency injection for service management".

## Code Reviews

docs/architecture.md should be referenced during code reviews to ensure adherence to these standards. Reviewers must verify that new code follows the established architecture, principles, and coding standards outlined here.
