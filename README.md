# cpp-sqlite

A modern C++20 SQLite wrapper that uses [Boost.Describe](https://www.boost.org/doc/libs/1_86_0/libs/describe/doc/html/describe.html) for compile-time reflection, automatically mapping C++ structs to database tables.

## Features

- **Zero-boilerplate table creation** — define a C++ struct, register it with `BOOST_DESCRIBE_STRUCT`, and the library auto-generates `CREATE TABLE` statements with proper column types, primary keys, and foreign key constraints
- **DAO pattern** — type-safe `DataAccessObject<T>` provides `insert()`, `selectAll()`, `selectById()`, and buffered bulk inserts
- **Foreign key relationships** — two modes: eager (auto-load nested objects) and lazy (`ForeignKey<T>` — stores only the ID until you call `resolve()`)
- **Repeated fields (one-to-many)** — `RepeatedFieldTransferObject<T>` handles junction tables automatically
- **Transactions with savepoints** — RAII `Transaction` class with commit/rollback and support for nested transactions via savepoints
- **Thread-safe buffered writes** — multiple threads can call `addToBuffer()` concurrently; a single `flushAllDAOs()` call inserts everything
- **BLOB support** — `std::vector<uint8_t>` members are stored as SQLite BLOBs
- **Namespace stripping** — `my_app::Product` becomes table `Product` automatically
- **CMake + Conan** — first-class Conan package with install rules for both headers and CMake config files

## Dependencies

| Dependency | Version | Purpose |
|-----------|---------|---------|
| C++20 compiler | — | Concepts, `constexpr`, CTAD |
| SQLite3 | 3.47 | Underlying database engine |
| Boost | 1.86 | Boost.Describe (reflection), Boost.MP11 (metaprogramming) |
| spdlog | 1.14 | Logging (injectable; optional) |
| Google Test | 1.14 | Test framework (test-only) |
| CMake | ≥ 3.22 | Build system |

## Quick Example

```cpp
#include "cpp_sqlite/src/cpp_sqlite/DBDatabase.hpp"
#include <boost/describe.hpp>

// 1. Define your data model
struct Product : public cpp_sqlite::BaseTransferObject {
    std::string name;
    float price;
    int quantity;
};

BOOST_DESCRIBE_STRUCT(Product, (cpp_sqlite::BaseTransferObject), (name, price, quantity));

// 2. Open a database
cpp_sqlite::Database db("store.db", /*allowWrite=*/true);

// 3. Get a type-safe DAO — auto-creates the table
auto& products = db.getDAO<Product>();

// 4. Insert records
Product widget;
widget.name = "Widget";
widget.price = 19.99f;
widget.quantity = 100;
products.addToBuffer(widget);
products.insert();

// 5. Query
auto all = products.selectAll();
for (const auto& p : all) {
    std::cout << p.name << ": $" << p.price << " (" << p.quantity << ")\n";
}
```

## Core Concepts

### Transfer Objects

Every database table is represented by a C++ struct that inherits from `BaseTransferObject`, which provides an `uint32_t id` primary key:

```cpp
struct User : public cpp_sqlite::BaseTransferObject {
    std::string name;
    std::string email;
};

BOOST_DESCRIBE_STRUCT(User, (cpp_sqlite::BaseTransferObject), (name, email));
```

Supported field types:
- `int`, `uint32_t`, `int64_t`, etc. (integer types) → `INTEGER`
- `float`, `double` → `FLOAT`
- `std::string` → `TEXT`
- `std::vector<uint8_t>` → `BLOB`
- Other transfer objects → foreign key column (`field_id INTEGER`)
- `ForeignKey<T>` → lazy foreign key
- `RepeatedFieldTransferObject<T>` → junction table

### Foreign Keys

Two modes for handling relationships:

**Eager loading** (default) — nested objects are automatically loaded during `SELECT`:

```cpp
struct Profile : public cpp_sqlite::BaseTransferObject {
    std::string bio;
};

struct User : public cpp_sqlite::BaseTransferObject {
    std::string name;
    Profile profile;  // Eager: full object loaded on SELECT
};
```

**Lazy loading** via `ForeignKey<T>` — only the ID is stored; the full object is loaded on demand:

```cpp
struct User : public cpp_sqlite::BaseTransferObject {
    std::string name;
    cpp_sqlite::ForeignKey<Profile> profile;  // Lazy: just stores ID
};

// Later, when you actually need the profile data:
auto user = userDAO.selectById(1);
if (auto profile = user->profile.resolve(db)) {
    std::cout << profile->get().bio << "\n";
}
```

### Repeated Fields (One-to-Many)

`RepeatedFieldTransferObject<T>` models one-to-many relationships using an auto-generated junction table:

```cpp
struct Tag : public cpp_sqlite::BaseTransferObject {
    std::string name;
};

struct Article : public cpp_sqlite::BaseTransferObject {
    std::string title;
    cpp_sqlite::RepeatedFieldTransferObject<Tag> tags;
};

BOOST_DESCRIBE_STRUCT(Tag, (cpp_sqlite::BaseTransferObject), (name));
BOOST_DESCRIBE_STRUCT(Article, (cpp_sqlite::BaseTransferObject), (title, tags));
BOOST_DESCRIBE_STRUCT(cpp_sqlite::RepeatedFieldTransferObject<Tag>, (), (data));
```

The library creates tables `Article`, `Tag`, and a junction table `Article_Tag(article_id, tag_id)`.

### Transactions

RAII transaction management with automatic rollback on scope exit:

```cpp
// Simple transaction
db.withTransaction([&]() {
    auto& dao = db.getDAO<Product>();
    dao.insert(product1);
    dao.insert(product2);
});  // Auto-commit on success, auto-rollback on exception

// Manual transaction
{
    cpp_sqlite::Transaction txn(db);
    // ... database operations ...
    txn.commit();  // or let destructor rollback
}

// Nested transactions use savepoints automatically:
{
    cpp_sqlite::Transaction outer(db);
    // ... insert some data ...
    {
        cpp_sqlite::Transaction inner(db);  // This becomes a SAVEPOINT
        // ... insert more data ...
        inner.rollback();  // Rolls back only the inner work
    }
    outer.commit();  // Outer data is committed
}
```

### Thread-Safe Buffered Writes

Multiple threads can add records to a buffer; a single thread flushes:

```cpp
auto& dao = db.getDAO<Product>();

// Multiple threads can call this safely:
std::thread t1([&]() { dao.addToBuffer(productA); });
std::thread t2([&]() { dao.addToBuffer(productB); });

t1.join();
t2.join();

// Single-threaded flush:
dao.insert();  // or db.flushAllDAOs() to flush all DAOs at once
```

### Logging

Inject an spdlog logger for observability:

```cpp
auto logger = spdlog::stdout_color_mt("my_app");
cpp_sqlite::Database db("app.db", true, logger);
// All SQL operations are now logged at debug/trace levels
```

## Building

### Via Conan (recommended as a dependency)

Add to your `conanfile.txt` or `conanfile.py`:

```python
def requirements(self):
    self.requires("cpp_sqlite/0.1.0")
```

Then in your CMakeLists.txt:

```cmake
find_package(cpp_sqlite REQUIRED)
target_link_libraries(your_target PRIVATE cpp_sqlite::cpp_sqlite)
```

Headers are available as:

```cpp
#include "cpp_sqlite/src/cpp_sqlite/DBDatabase.hpp"
```

### Standalone build

```bash
# Install dependencies
conan install . --build=missing -s build_type=Release

# Build
cmake --preset conan-release
cmake --build --preset conan-release

# Run tests
ctest --preset conan-release
```

## Project Structure

```
cpp-sqlite/
├── cpp_sqlite/
│   ├── src/
│   │   ├── cpp_sqlite/
│   │   │   ├── DBBaseTransferObject.hpp    # Base struct with `id` field
│   │   │   ├── DBTraits.hpp                # Concepts and type traits
│   │   │   ├── DBDatabase.hpp              # Database class + generic select/insert
│   │   │   ├── DBDataAccessObject.hpp      # Type-safe DAO template
│   │   │   ├── DBDAOBase.hpp               # DAO base class (type-erased)
│   │   │   ├── DBForeignKey.hpp            # Lazy FK wrapper
│   │   │   ├── DBRepeatedFieldTransferObject.hpp  # One-to-many container
│   │   │   └── DBTransaction.hpp           # RAII transaction
│   │   └── utils/
│   │       ├── Logger.hpp/cpp              # spdlog singleton wrapper
│   │       └── StringUtils.hpp             # Namespace stripping
│   └── test/
│       └── testDatabase.cpp                # Comprehensive test suite
├── CMakeLists.txt                          # Top-level CMake
├── conanfile.py                           # Conan package recipe
└── Config.cmake.in                        # CMake package config template
```

## License

MIT — see [conanfile.py](conanfile.py).
