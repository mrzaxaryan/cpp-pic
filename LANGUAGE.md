# PIL (Position Independent Language) Specification

PIL (Position Independent Language) is a lightweight, position-independent scripting language embedded in the Position-Independent Runtime (PIR). It provides a State-based API while maintaining full position-independence with no `.rdata` dependencies.

## Table of Contents

- [Overview](#overview)
- [Lexical Structure](#lexical-structure)
- [Data Types](#data-types)
- [Variables](#variables)
- [Operators](#operators)
- [Control Flow](#control-flow)
- [Functions](#functions)
- [Arrays](#arrays)
- [Standard Library](#standard-library)
- [File I/O](#file-io)
- [Network I/O](#network-io)
- [C++ Integration](#c-integration)
- [Error Handling](#error-handling)
- [Constraints](#constraints)

---

## Overview

| Property | Value |
|----------|-------|
| **Name** | PIL |
| **Type** | Dynamically typed, interpreted |
| **Target** | Embedded in cpp-pic runtime |
| **Dependencies** | None (no .rdata, no CRT) |

### Design Philosophy

- **No built-in functions**: All functions must be registered from C++
- **Position-independent**: Uses `_embed` strings throughout
- **Minimal footprint**: Designed for shellcode and embedded contexts
- **State-based API**: Familiar State-based interface

---

## Lexical Structure

### Comments

```javascript
// Single-line comment

/* Multi-line
   comment */
```

### Keywords

```
var     if      else    while   for     fn
return  break   continue true    false   nil
in
```

### Identifiers

Identifiers start with a letter or underscore, followed by letters, digits, or underscores:

```
myVar
_private
count123
```

### Literals

**Numbers (integers and floats):**
```javascript
42
-17
0
3.14
0.5
-2.718
```

**Strings:**
```javascript
"hello world"
"escape sequences: \n \t \\ \""
""  // empty string
```

**Booleans:**
```javascript
true
false
```

**Nil:**
```javascript
nil
```

**Arrays:**
```javascript
[1, 2, 3]
["a", "b", "c"]
[1, "mixed", true, nil]
[]  // empty array
```

---

## Data Types

PIL has six value types:

| Type | Description | Example |
|------|-------------|---------|
| `nil` | Absence of value | `nil` |
| `bool` | Boolean | `true`, `false` |
| `number` | 64-bit floating point (DOUBLE) | `42`, `3.14`, `-2.5` |
| `string` | Character sequence | `"hello"` |
| `array` | Indexed collection | `[1, 2, 3]` |
| `function` | Callable | `fn(x) { return x; }` |

### Type Coercion

- **Truthiness**: `nil` and `false` are falsy; everything else is truthy
- **String concatenation**: `+` with strings performs concatenation
- **Numeric operations**: Non-numbers in arithmetic cause runtime errors

---

## Variables

### Declaration

Variables are declared with `var`:

```javascript
var x = 10;
var name = "hello";
var flag = true;
var empty;  // initialized to nil
```

### Assignment

```javascript
x = 20;
name = "world";
```

### Compound Assignment

```javascript
x += 5;   // x = x + 5
x -= 3;   // x = x - 3
x *= 2;   // x = x * 2
x /= 4;   // x = x / 4
```

### Scope

Variables follow lexical scoping:

```javascript
var x = "outer";
{
    var x = "inner";  // shadows outer x
    print(x);         // "inner"
}
print(x);             // "outer"
```

Maximum scope depth: 32 levels.

---

## Operators

### Precedence (highest to lowest)

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 | `()` `[]` | Left-to-right |
| 2 | `!` `-` (unary) | Right-to-left |
| 3 | `*` `/` `%` | Left-to-right |
| 4 | `+` `-` | Left-to-right |
| 5 | `<` `>` `<=` `>=` | Left-to-right |
| 6 | `==` `!=` | Left-to-right |
| 7 | `&&` | Left-to-right |
| 8 | `\|\|` | Left-to-right |
| 9 | `=` `+=` `-=` `*=` `/=` | Right-to-left |

### Arithmetic Operators

```javascript
var a = 10 + 5;   // 15
var b = 10 - 5;   // 5
var c = 10 * 5;   // 50
var d = 10 / 4;   // 2.5 (float division)
var e = 10 % 3;   // 1
var f = -a;       // -15
var g = 3.14 * 2; // 6.28
```

### Comparison Operators

```javascript
a == b    // equal
a != b    // not equal
a < b     // less than
a > b     // greater than
a <= b    // less than or equal
a >= b    // greater than or equal
```

### Logical Operators

```javascript
a && b    // logical AND (short-circuit)
a || b    // logical OR (short-circuit)
!a        // logical NOT
```

### String Concatenation

```javascript
var greeting = "Hello, " + "World!";  // "Hello, World!"
var msg = "Value: " + str(42);        // "Value: 42"
```

---

## Control Flow

### If Statement

```javascript
if (condition) {
    // then branch
}

if (condition) {
    // then branch
} else {
    // else branch
}

if (condition1) {
    // branch 1
} else if (condition2) {
    // branch 2
} else {
    // else branch
}
```

### While Loop

```javascript
var i = 0;
while (i < 10) {
    print(i);
    i = i + 1;
}
```

### For Loop (Traditional)

```javascript
for (var i = 0; i < 10; i = i + 1) {
    print(i);
}
```

### For-Each Loop

Iterate over arrays:
```javascript
var arr = [1, 2, 3];
for (var item in arr) {
    print(item);
}
```

Iterate over strings:
```javascript
for (var c in "hello") {
    print(c);
}
```

With index:
```javascript
for (var i, item in arr) {
    print(i, ":", item);
}
```

### Break and Continue

```javascript
for (var i = 0; i < 10; i = i + 1) {
    if (i == 5) {
        break;      // exit loop
    }
    if (i % 2 == 0) {
        continue;   // skip to next iteration
    }
    print(i);
}
```

---

## Functions

### Declaration

```javascript
fn add(a, b) {
    return a + b;
}

fn greet(name) {
    print("Hello, " + name + "!");
}

fn factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

### Calling Functions

```javascript
var sum = add(3, 4);      // 7
greet("World");           // prints "Hello, World!"
var f = factorial(5);     // 120
```

### Return Statement

```javascript
fn getValue() {
    return 42;
}

fn earlyReturn(x) {
    if (x < 0) {
        return nil;  // early return
    }
    return x * 2;
}
```

Functions without a `return` statement return `nil`.

### First-Class Functions

Functions are first-class values:

```javascript
fn apply(f, x) {
    return f(x);
}

fn double(n) {
    return n * 2;
}

var result = apply(double, 21);  // 42
```

---

## Arrays

### Creation

```javascript
var empty = [];
var numbers = [1, 2, 3, 4, 5];
var mixed = [1, "two", true, nil];
```

Maximum 16 elements per array. Maximum 64 arrays in pool.

### Access

```javascript
var arr = [10, 20, 30];
print(arr[0]);    // 10
print(arr[2]);    // 30
```

String indexing:
```javascript
var s = "hello";
print(s[0]);      // "h"
print(s[4]);      // "o"
```

### Assignment

```javascript
var arr = [1, 2, 3];
arr[1] = 99;
print(arr);       // [1, 99, 3]
```

### Array Functions

```javascript
var arr = [1, 2, 3];

print(len(arr));      // 3

push(arr, 4);         // arr is now [1, 2, 3, 4]

var last = pop(arr);  // last = 4, arr is [1, 2, 3]
```

---

## Standard Library

Register the standard library:
```cpp
PIL::State L;
PIL::OpenStdLib(L);
```

### Available Functions

| Function | Description | Example | Result |
|----------|-------------|---------|--------|
| `print(...)` | Print values to console | `print("x =", 42);` | Output: `x = 42` |
| `len(v)` | Get length of string/array | `len("hello")` | `5` |
| `str(v)` | Convert to string | `str(3.14)` | `"3.14"` |
| `num(v)` | Convert to number | `num("3.14")` | `3.14` |
| `type(v)` | Get type name | `type(42)` | `"number"` |
| `abs(n)` | Absolute value | `abs(-5.5)` | `5.5` |
| `min(a, b)` | Minimum of two values | `min(3, 5)` | `3` |
| `max(a, b)` | Maximum of two values | `max(3, 5)` | `5` |
| `floor(n)` | Round down to nearest integer | `floor(3.7)` | `3` |
| `ceil(n)` | Round up to nearest integer | `ceil(3.2)` | `4` |
| `int(n)` | Truncate to integer (toward zero) | `int(-3.7)` | `-3` |
| `push(arr, v)` | Add element to array | `push(arr, 4)` | - |
| `pop(arr)` | Remove last element | `pop(arr)` | Last element |

---

## File I/O

File operations are available through the standard library.

### Functions

| Function | Description |
|----------|-------------|
| `fopen(path, mode)` | Open file. Modes: `"r"`, `"w"`, `"a"`, `"rb"`, `"wb"`, `"ab"` |
| `fclose(handle)` | Close file |
| `fread(handle [, size])` | Read from file (max 255 bytes per call) |
| `freadline(handle)` | Read a line from file |
| `fwrite(handle, data)` | Write to file |
| `fexists(path)` | Check if file exists |
| `fdelete(path)` | Delete file |
| `fsize(handle)` | Get file size |
| `fseek(handle, offset, origin)` | Set position (0=start, 1=current, 2=end) |
| `ftell(handle)` | Get current position |
| `mkdir(path)` | Create directory |
| `rmdir(path)` | Remove directory |

Maximum 16 open files simultaneously.

### Example

```javascript
// Write to file
var f = fopen("test.txt", "w");
if (f != nil) {
    fwrite(f, "Hello, World!\n");
    fwrite(f, "Line 2\n");
    fclose(f);
}

// Read from file
f = fopen("test.txt", "r");
if (f != nil) {
    var line = freadline(f);
    while (line != nil) {
        print(line);
        line = freadline(f);
    }
    fclose(f);
}

// Check file exists
if (fexists("test.txt")) {
    print("File exists!");
}
```

---

## Network I/O

Network operations are available through the network I/O library.

### Setup

```cpp
PIL::NetworkContext netCtx;
PIL::State L;
PIL::OpenStdLib(L);
PIL::OpenNetworkIO(L, &netCtx);
```

### Socket Functions

| Function | Description |
|----------|-------------|
| `sock_connect(host, port)` | Connect to host:port, returns handle or -1 |
| `sock_close(handle)` | Close socket, returns true/false |
| `sock_send(handle, data)` | Send data, returns bytes sent or -1 |
| `sock_recv(handle [, size])` | Receive data (max 255 bytes), returns string |

Maximum 8 sockets simultaneously.

#### Example

```javascript
var sock = sock_connect("httpbin.org", 80);
if (sock >= 0) {
    sock_send(sock, "GET / HTTP/1.0\r\nHost: httpbin.org\r\n\r\n");
    var response = sock_recv(sock, 255);
    print(response);
    sock_close(sock);
}
```

### DNS Functions

| Function | Description |
|----------|-------------|
| `dns_resolve(hostname)` | Resolve hostname to IP string (IPv6 preferred) |
| `dns_resolve4(hostname)` | Resolve hostname to IPv4 string |
| `dns_resolve6(hostname)` | Resolve hostname to IPv6 string |

#### Example

```javascript
var ip = dns_resolve("cloudflare.com");
print("IP:", ip);

var ipv4 = dns_resolve4("example.com");
var ipv6 = dns_resolve6("example.com");
```

### HTTP Functions

| Function | Description |
|----------|-------------|
| `http_open(url)` | Create HTTP client for URL, returns handle or -1 |
| `http_get(handle)` | Send GET request, returns true/false |
| `http_post(handle, data)` | Send POST request, returns true/false |
| `http_read(handle [, size])` | Read response (max 255 bytes), returns string |
| `http_close(handle)` | Close HTTP client, returns true/false |

Maximum 4 HTTP clients simultaneously.

#### Example

```javascript
var http = http_open("http://httpbin.org/ip");
if (http >= 0) {
    if (http_get(http)) {
        var response = "";
        var chunk = http_read(http, 255);
        while (len(chunk) > 0) {
            response = response + chunk;
            chunk = http_read(http, 255);
        }
        print(response);
    }
    http_close(http);
}
```

### WebSocket Functions

| Function | Description |
|----------|-------------|
| `ws_connect(url)` | Connect to WebSocket server (ws:// or wss://), returns handle or -1 |
| `ws_close(handle)` | Close WebSocket connection, returns true/false |
| `ws_send(handle, data [, opcode])` | Send data with optional opcode, returns bytes sent or -1 |
| `ws_send_text(handle, data)` | Send text data (opcode=1), returns bytes sent or -1 |
| `ws_recv(handle [, size])` | Receive data (max 255 bytes), returns string |
| `ws_ping(handle)` | Send ping frame, returns true/false |
| `ws_pong(handle)` | Send pong frame, returns true/false |

Maximum 4 WebSocket connections simultaneously.

**Opcodes:**
| Opcode | Description |
|--------|-------------|
| 0 | Continuation |
| 1 | Text |
| 2 | Binary (default) |
| 8 | Close |
| 9 | Ping |
| 10 | Pong |

#### Example

```javascript
// Connect to WebSocket echo server
var ws = ws_connect("wss://echo.websocket.org");
if (ws >= 0) {
    // Send text message
    ws_send_text(ws, "Hello WebSocket!");

    // Receive echo response
    var data = ws_recv(ws);
    print("Received:", data);

    // Send binary data with explicit opcode
    ws_send(ws, "binary data", 2);

    // Send ping
    ws_ping(ws);

    // Close connection
    ws_close(ws);
}
```

#### Real-time Communication Example

```javascript
var ws = ws_connect("wss://myserver.com/socket");
if (ws >= 0) {
    // Send JSON message
    ws_send_text(ws, "{\"type\":\"subscribe\",\"channel\":\"updates\"}");

    // Read messages in a loop
    var running = true;
    while (running) {
        var msg = ws_recv(ws);
        if (len(msg) > 0) {
            print("Message:", msg);
            // Process message...
        }
    }

    ws_close(ws);
}
```

---

## C++ Integration

### Basic Usage

```cpp
#include "pil/pil.h"

PIL::State* L = new PIL::State();

// Register standard library
PIL::OpenStdLib(*L);

// Execute script
L->DoString(R"(
    print("Hello from PIL!");
)"_embed);

delete L;
```

### Custom Functions

```cpp
PIL::Value MyFunction(PIL::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsNumber(0))
    {
        INT64 n = ctx.ToNumber(0);
        return PIL::Value::Number(n * 2);
    }
    return PIL::Value::Nil();
}

// Register custom function
L->Register("double"_embed, MyFunction);
```

### Setting Global Variables

```cpp
L->SetGlobalNumber("count"_embed, 5, 42);
L->SetGlobalFloat("PI"_embed, 2, DOUBLE::FromParts(3, 141592));  // 3.141592
L->SetGlobalString("version"_embed, 7, "1.0.0"_embed, 5);
L->SetGlobalBool("debug"_embed, 5, TRUE);
```

### FunctionContext API

| Method | Description |
|--------|-------------|
| `ArgCount()` | Get number of arguments |
| `CheckArgs(n)` | Check if exactly n arguments |
| `IsNil(i)` | Check if argument i is nil |
| `IsBool(i)` | Check if argument i is bool |
| `IsNumber(i)` | Check if argument i is number |
| `IsString(i)` | Check if argument i is string |
| `IsArray(i)` | Check if argument i is array |
| `ToBool(i)` | Get argument i as bool |
| `ToNumber(i)` | Get argument i as number |
| `ToString(i)` | Get argument i as string |

### Value Creation

```cpp
PIL::Value::Nil()
PIL::Value::Bool(true)
PIL::Value::Number(42)
PIL::Value::String("hello", 5)
```

---

## Error Handling

### In Scripts

Errors halt execution and set an error message:

```cpp
if (!L->DoString(source))
{
    Console::Write<CHAR>("Error: "_embed);
    Console::Write<CHAR>(L->GetError());
}
```

### Common Errors

| Error | Cause |
|-------|-------|
| `Undefined variable 'x'` | Using undeclared variable |
| `Division by zero` | Dividing by zero |
| `Type error` | Invalid operation for types |
| `Index out of bounds` | Array/string index invalid |
| `Too many arguments` | Exceeding function parameter limit |
| `break outside loop` | Using break outside a loop |
| `continue outside loop` | Using continue outside a loop |

---

## Constraints

Due to cpp-pic compatibility requirements:

1. **No .rdata**: All strings use `_embed` suffix
2. **No exceptions**: Uses error codes/flags
3. **No dynamic allocation**: Fixed-size buffers and pools
4. **No STL**: Custom containers only
5. **Position-independent**: No relocation dependencies

### Limits

| Resource | Limit |
|----------|-------|
| Scope depth | 32 levels |
| Array elements | 16 per array |
| Array pool | 64 arrays |
| Open files | 16 handles |
| Open sockets | 8 handles |
| HTTP clients | 4 handles |
| WebSocket clients | 4 handles |
| String buffer | 256 characters |

---

## Examples

### FizzBuzz

```javascript
fn fizzbuzz(n) {
    for (var i = 1; i <= n; i = i + 1) {
        if (i % 15 == 0) {
            print("FizzBuzz");
        } else if (i % 3 == 0) {
            print("Fizz");
        } else if (i % 5 == 0) {
            print("Buzz");
        } else {
            print(i);
        }
    }
}
fizzbuzz(15);
```

### Fibonacci

```javascript
fn fib(n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

for (var i = 0; i < 10; i = i + 1) {
    print(fib(i));
}
```

### Array Processing

```javascript
var numbers = [1, 2, 3, 4, 5];
var sum = 0;

for (var n in numbers) {
    sum = sum + n;
}

print("Sum:", sum);           // Sum: 15
print("Average:", sum / len(numbers));  // Average: 3.0
```

### Floating-Point Math

```javascript
var pi = 3.14159;
var radius = 2.5;
var area = pi * radius * radius;
print("Area:", area);         // Area: 19.634...

var x = 7.8;
print("floor:", floor(x));    // floor: 7
print("ceil:", ceil(x));      // ceil: 8
print("int:", int(-3.9));     // int: -3 (truncates toward zero)
```

### File Processing

```javascript
// Count lines in a file
fn countLines(path) {
    var f = fopen(path, "r");
    if (f == nil) {
        return -1;
    }

    var count = 0;
    var line = freadline(f);
    while (line != nil) {
        count = count + 1;
        line = freadline(f);
    }

    fclose(f);
    return count;
}

var lines = countLines("myfile.txt");
print("Lines:", lines);
```

### HTTP Request

```javascript
// Fetch data from an API
fn httpGet(url) {
    var http = http_open(url);
    if (http < 0) {
        return nil;
    }

    if (!http_get(http)) {
        http_close(http);
        return nil;
    }

    var response = "";
    var chunk = http_read(http, 255);
    while (len(chunk) > 0) {
        response = response + chunk;
        chunk = http_read(http, 255);
    }

    http_close(http);
    return response;
}

var data = httpGet("http://httpbin.org/ip");
if (data != nil) {
    print("Response:", data);
}
```

### WebSocket Chat

```javascript
// Simple WebSocket echo client
var ws = ws_connect("wss://echo.websocket.org");
if (ws >= 0) {
    print("Connected to echo server");

    // Send messages
    var messages = ["Hello", "World", "Test"];
    for (var msg in messages) {
        ws_send_text(ws, msg);
        print("Sent:", msg);

        var reply = ws_recv(ws);
        print("Echo:", reply);
    }

    ws_close(ws);
    print("Disconnected");
} else {
    print("Connection failed");
}
```

---

## Architecture

PIL is implemented as a standalone module:

```
include/pil/
├── token.h          # Token types and struct
├── lexer.h          # Lexer class
├── ast.h            # AST node definitions + allocator
├── parser.h         # Recursive descent parser
├── value.h          # Value type system + Environment
├── interpreter.h    # Tree-walking interpreter
├── stdlib.h         # Standard library functions
├── fileio.h         # File I/O functions
├── networkio.h      # Network I/O functions (socket, DNS, HTTP, WebSocket)
├── state.h          # State-based API wrapper
└── pil.h            # Main entry point (include this)
```
