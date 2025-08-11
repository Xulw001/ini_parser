## Description

A simple header-only INI parser using modern C++ implementations. It is easy to use and simplifies working with INI files.

## Usage

#### 1. read

```c++
#include "ini.h"

int main()
{
    ini::parser::config cf;
    ini::parser::read_ini("app.ini", cf);
    // if section or key not exist, default value will been returned
    auto ip = cf.get<std::string>("common", "ip");
    auto port = cf.get<int>("common", "port", 22);
    return 0;
}
```

#### 2. write

```c++
#include "ini.h"
int main()
{
    ini::parser::config cf;
    // update the config when key or section exist otherwise add
    cf.set("common", "ip", "localhost");
    cf.set("Test", "port", 23);
    ini::parser::write_ini("app.ini", cf);
    return 0;
}
```

## Shortcomings

- Only support INI file with ANSI on windows，UTF-8 on Linux.
- Don't support traversal.

