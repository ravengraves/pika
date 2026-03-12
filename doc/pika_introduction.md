# Introduction to the Pika Programming Language
---

## Summary
Pika is a small, safety-oriented subset of Hare designed for predictable
systems programming. In this repository, Pika is transpiled to GNU C11 by
`pika2c`, then compiled with a normal C compiler and linked with a tiny
runtime.

> Disclaimer, I build robots, so I made Pika to run Hare on embedded systems.

---

Pika is intentionally small: fewer features, fewer sharp edges, and fewer ways
to surprise yourself six months later. If you've read Hare's "100-year language"
essay, you already know the goals: conservative design and long-term
maintainability first. (And admittedly a simpler transpiler than full Hare)

This guide is written for the `pika2c` implementation in this repository.

## Setup and Compilation

The `pikacc` script handles the full compilation pipeline: it transpiles your
`.pka` source to C, links in the standard library and runtime, and produces a
native binary.

```pika
// hello.pka
use rt;

export fn main() void = {
	let msg: [_]u8 = ['H', 'e', 'l', 'l', 'o', ',', ' ', 'P', 'i', 'k', 'a', '!', '\n'];
	rt::write(1, &msg: *opaque, len(msg));
};
```

```bash
bin/pikacc hello.pka -o hello
./hello
```

Under the hood, `pikacc` runs `pika2c` (the transpiler) to generate GNU C11,
then compiles it with `gcc` or `clang` and links with `runtime/rt.c`. You can
override the C compiler with `CC=clang bin/pikacc ...`.

For more examples, browse `test/*.pka` and `test/stdlib/*.pka`. To run the
full test suite, use `ninja check`.

## The Pika Mindset

Pika is a stricter subset of Hare. It keeps familiar syntax (functions, structs,
arrays, enums, tagged unions) but removes features that complicate reasoning
about correctness and resource usage: no recursion, no dynamic allocation
keywords, no function pointers.

```pika
// Iteration is the default; recursion is rejected by pika2c.
fn factorial(n: i32) i32 = {
	let result: i32 = 1;
	for (let i: i32 = 2; i <= n; i += 1) {
		result *= i;
	};
	return result;
};

export fn main() void = {
	assert(factorial(0) == 1);
	assert(factorial(5) == 120);
};
```

The design goal is not "max power," it's "max confidence." Every function call
is a plain call (no vtables, no closures). Every loop is a `for`. Every
allocation is visible in the source as a local variable or global. When you read
Pika code, the execution cost of each line is obvious.

## Bindings: `def`, `const`, `let`, and `static let`

Pika has four ways to name a value:

- `def NAME: T = ...;` — compile-time constant (useful for sizes and flags)
- `const name: T = ...;` — immutable binding (global or local)
- `let name: T = ...;` — mutable binding (global or local)
- `static let name: T = ...;` — function-local static storage (persists across calls)

Pika forbids variable shadowing: you can't reuse a name in an inner scope.

```pika
def MAX_TICKS: i32 = 4;
const START_AT: i32 = 10;
let global_ticks: i32 = 0;

fn tick() i32 = {
	static let local_ticks: i32 = 0;
	local_ticks += 1;
	global_ticks += 1;
	return START_AT + local_ticks + global_ticks;
};

export fn main() void = {
	let sum: i32 = 0;
	for (let i: i32 = 0; i < MAX_TICKS; i += 1) {
		sum += tick();
	};
	assert(global_ticks == MAX_TICKS);
	assert(sum == 60);
};
```

The no-shadowing rule means you never have to wonder "which `count` is this?"
<<<<<<< Updated upstream
- there is only one `count` in scope. For state, prefer plain locals first,
=======
— there is only one `count` in scope. For state, prefer plain locals first,
>>>>>>> Stashed changes
then `static let` for persistent function-local state, and globals only when
the state is truly shared.

## Types and Literals

Pika supports fixed-width integers (`i32`, `u64`), `bool`, floats (`f32`,
`f64`), pointer-sized types (`size`, `uintptr`), plus composite types: arrays,
slices, structs, enums, and tagged unions. Integer literals can be suffixed
(`10u8`, `0z`), and you cast with `expr: Type`.

```pika
type point = struct {
	x: i32,
	y: i32,
};

export fn main() void = {
	let answer: i32 = 42;
	let byte: u8 = 10u8;
	let count: size = 0z;
	let piish: f32 = 3: f32;
	let newline: u8 = '\n';
	let good = true;

	let p = point { x = answer, y = 1 };
	let name: [_]u8 = ['P', 'i', 'k', 'a'];

	assert(len(name) == 4);
	assert(size(i32) == 4);
	assert(byte == 10);
	assert(count == 0);
	assert(piish: i32 == 3);
	assert(newline == 10u8);
	assert(p.x == 42);
	assert(good == true);
};
```

`size(T)` tells you how many bytes a type occupies. Casts are always written
<<<<<<< Updated upstream
with `:` so they stand out in code review - there are no implicit conversions.
=======
with `:` so they stand out in code review — there are no implicit conversions.
>>>>>>> Stashed changes

## Functions

Functions are declared with `fn name(params...) ReturnType = { ... };`. Use
`export fn` to expose a function to the generated C interface. The program's
entry point is `export fn main() void`, which `pika2c` maps to `_main` for the
runtime.

```pika
type op = enum {
	ADD,
	SUB,
};

fn apply(kind: op, a: i32, b: i32) i32 = {
	switch (kind) {
	case op::ADD => return a + b;
	case op::SUB => return a - b;
	};
};

export fn main() void = {
	assert(apply(op::ADD, 10, 20) == 30);
	assert(apply(op::SUB, 10, 20) == -10);
};
```

No default parameters, no variadics, no function pointers. Call sites are
always explicit: if a function needs data, you pass it. If you want "pick one
of several behaviors," model it with an enum and a `switch`.

## Control Flow

Pika keeps control flow simple: `if`/`else`, `for` loops, `switch`, and `match`.
<<<<<<< Updated upstream
`if` is a statement, not an expression - assign in each branch when you need a
=======
`if` is a statement, not an expression — assign in each branch when you need a
>>>>>>> Stashed changes
value. `yield` is only used inside `match` expressions.

```pika
fn sum_even_under(limit: i32) i32 = {
	let total: i32 = 0;
	let i: i32 = 0;
	for (true) {
		if (i >= limit) {
			break;
		};
		if ((i & 1) != 0) {
			i += 1;
			continue;
		};
		total += i;
		i += 1;
	};
	return total;
};

export fn main() void = {
	assert(sum_even_under(6) == 0 + 2 + 4);
};
```

Every branch and loop maps directly to the machine instructions you'd expect.
No hidden iterator protocols, no lazy evaluation, no implicit unwinding. When
you read a `for` loop, you know exactly what it does.

## Arrays and Slices

Arrays (`[N]T` and `[_]T`) are fixed-size values. Slices (`[]T`) are views into
<<<<<<< Updated upstream
arrays - a pointer plus a length. Indexing is bounds-checked at runtime:
=======
arrays — a pointer plus a length. Indexing is bounds-checked at runtime:
>>>>>>> Stashed changes
out-of-range access aborts.

```pika
fn sum(items: []i32) i32 = {
	let total: i32 = 0;
	for (let i = 0z; i < len(items); i += 1) {
		total += items[i];
	};
	return total;
};

export fn main() void = {
	let data: [6]i32 = [10, 20, 30, 40, 50, 60];
	let middle = data[2..5]; // [30, 40, 50]
	let tail = data[3..];    // [40, 50, 60]

	assert(len(middle) == 3);
	assert(middle[0] == 30);
	assert(sum(middle) == 120);
	assert(sum(tail) == 150);
};
```

<<<<<<< Updated upstream
Slices don't allocate and don't own memory - they point at an existing region.
=======
Slices don't allocate and don't own memory — they point at an existing region.
>>>>>>> Stashed changes
The standard pattern is: keep data in an array, take slices to pass into
functions. No copies, no hidden allocations, and the bounds check catches
off-by-one errors at the point of access.

## Pointers and Auto-Dereference

Pointers are written `*T`, address-of is `&x`, dereference is `*p`. For field
access, Pika auto-dereferences: if `p: *point`, then `p.x` works (generates
`p->x` in C). This applies through multiple pointer levels and to
pointers-to-arrays.

```pika
type point = struct {
	x: i32,
	y: i32,
};

fn manhattan(p: *point) i32 = {
	return p.x + p.y;
};

export fn main() void = {
	let pt = point { x = 3, y = 4 };
	assert(manhattan(&pt) == 7);

	let nums: [3]i32 = [10, 20, 30];
	let pnums: *[3]i32 = &nums;
	assert(pnums[1] == 20);
};
```

You still opt into pointers explicitly, but field access doesn't become a sea
of `*` and parentheses. The auto-deref is predictable: it only applies where
<<<<<<< Updated upstream
it's unambiguous - field access and indexing.
=======
it's unambiguous — field access and indexing.
>>>>>>> Stashed changes

## Nullable Pointers and Narrowing with `match`

Pointers are non-nullable by default. If a pointer may be absent, you must say
<<<<<<< Updated upstream
so with `nullable *T`. Nullable pointers cannot be dereferenced directly - you
=======
so with `nullable *T`. Nullable pointers cannot be dereferenced directly — you
>>>>>>> Stashed changes
narrow them via `match`:

```pika
fn deref_or(p: nullable *i32, fallback: i32) i32 = {
	return match (p) {
	case let q: *i32 =>
		yield *q;
	case null =>
		yield fallback;
	};
};

export fn main() void = {
	let x: i32 = 42;
	assert(deref_or(&x, 0) == 42);
	assert(deref_or(null, 0) == 0);
};
```

The rule is simple: you may only dereference `*T`, never `nullable *T`. The
idiom is to `match` once at the boundary, bind the non-null pointer in the
success arm, and use it freely inside. This turns "maybe null" into "definitely
not null" in a way the compiler enforces.

## Enums and `switch`

Enums are declared with `enum { ... }` (optionally with an underlying integer
<<<<<<< Updated upstream
type, e.g. `enum u8 { ... }`). Use `switch` to branch on enum values - it must
=======
type, e.g. `enum u8 { ... }`). Use `switch` to branch on enum values — it must
>>>>>>> Stashed changes
be exhaustive unless you add `case =>` as a default.

```pika
type mood = enum {
	SUN,
	RAIN,
};

fn mood_score(m: mood) i32 = {
	switch (m) {
	case mood::SUN => return 10;
	case mood::RAIN => return 2;
	};
};

export fn main() void = {
	assert(mood_score(mood::SUN) == 10);
	assert(mood_score(mood::RAIN) == 2);
};
```

Enums pair naturally with `switch`: you see all possible states and how they're
handled in one place. When you want "pick behavior at runtime" without function
pointers, an enum + switch is the standard pattern.

## Tagged Unions and `match` on Type

Tagged unions are written `(T | U | ...)`. You construct a variant with a cast
(`value: union_type`) and inspect it with `match`, using `case Type =>` arms.
A `match` must be exhaustive unless you include a default `case =>`.

```pika
type err = !void;

type u32_box = struct {
	v: u32,
};

type number = (i32 | f32 | u32_box | err);

fn to_i32(n: number) i32 = {
	match (n) {
	case i32 => return n: i32;
	case f32 => return (n: f32): i32;
	case u32_box => return (n: u32_box).v: i32;
	case err => return -1;
	};
};

export fn main() void = {
	let a: number = 42: number;
	let b: number = (3.5: f32): number;
	let c = u32_box { v = 7u32 }: number;
	let d = err;

	assert(to_i32(a) == 42);
	assert(to_i32(b) == 3);
	assert(to_i32(c) == 7);
	assert(to_i32(d) == -1);
};
```

Tagged unions are Pika's main tool for "either/or" values: optional results,
explicit errors, and polymorphic data. They carry a runtime tag, and `match`
forces you to handle every possibility. You'll see them throughout the standard
library for error handling (e.g. `file::OpenResult = (i32 | file::FileError)`).

## Strings (`str`) and the `strings` Module

`str` is a first-class type: a pointer plus a byte length. String literals
support escapes (`"\n"`, `"\x41"`, `"\u00E9"`), raw strings using backticks,
multi-line strings, concatenation by adjacency (`"a" "b"`), and equality
(`==`, `!=`).

```pika
use strings;

fn first_byte(s: str) u8 = {
	let bytes: []u8 = strings::toutf8(s);
	return bytes[0];
};

export fn main() void = {
	let a = "Hello, " "world!";
	let b = `Hello, world!`;

	assert(a == b);
	assert(len(a) == 13);
	assert(first_byte(a) == 'H');
};
```

To work with the raw bytes of a string, convert to `[]u8` with
<<<<<<< Updated upstream
`strings::toutf8`. The conversion is a view - no allocation, no copy.
To go back, use `strings::fromutf8_unsafe`.

## Output with `fb`

The `fb` module provides two levels of output. For simple cases, print a
string directly:

```pika
use fb;

export fn main() void = {
	fb::Prints("name: ");
	fb::Printsln("Pika");  // prints "Pika\n"
=======
`strings::toutf8`. The conversion is a view — no allocation, no copy.
To go back, use `strings::fromutf8_unsafe`.

## Output with `fmt`

The `fmt` module provides two levels of output. For simple cases, print a
string directly:

```pika
use fmt;

export fn main() void = {
	fmt::Prints("name: ");
	fmt::Printsln("Pika");  // prints "Pika\n"
>>>>>>> Stashed changes
};
```

For formatted output, you provide a `[]u8` buffer and build the output
<<<<<<< Updated upstream
explicitly. Nothing is allocated - you control exactly where the bytes go:

```pika
use fb;

export fn main() void = {
	let storage: [256]u8 = [0...];
	let b: fb::Buffer = fb::New(storage[0z..256z]);

	fb::Write(&b, "count: ");
	fb::Wi64(&b, 42);
	fb::Write(&b, ", pi: ");
	fb::Wf64(&b, 3.14159f64, 4);
	fb::Println(&b);  // flushes buffer to stdout + newline

	// F() does %-substitution (up to 5 args)
	fb::Clear(&b);
	fb::F(&b, "%s + %s = %s", "1", "2", "3");
	fb::Println(&b);  // prints "1 + 2 = 3\n"
};
```

`fb::Buffer` is a struct holding a `[]u8` slice and a write index. Functions
=======
explicitly. Nothing is allocated — you control exactly where the bytes go:

```pika
use fmt;

export fn main() void = {
	let storage: [256]u8 = [0...];
	let b: fmt::Buffer = fmt::New(storage[0z..256z]);

	fmt::Write(&b, "count: ");
	fmt::Wi64(&b, 42);
	fmt::Write(&b, ", pi: ");
	fmt::Wf64(&b, 3.14159f64, 4);
	fmt::Println(&b);  // flushes buffer to stdout + newline

	// F() does %-substitution (up to 5 args)
	fmt::Clear(&b);
	fmt::F(&b, "%s + %s = %s", "1", "2", "3");
	fmt::Println(&b);  // prints "1 + 2 = 3\n"
};
```

`fmt::Buffer` is a struct holding a `[]u8` slice and a write index. Functions
>>>>>>> Stashed changes
like `Wi64`, `Wu64`, `Wf64`, and `Wbool` format values into the buffer.
`Print` writes the buffer contents to stdout; `Println` adds a newline.
`Clear` resets the index to zero so you can reuse the same buffer.

This is the Pika way: you see the buffer, you see the writes, you see the
flush. No hidden heap allocations, no growable strings, no format-string
parsing at runtime.

## File I/O

The `file` module wraps platform file operations. Functions return tagged unions
<<<<<<< Updated upstream
for error handling - use the `Must*` helpers to unwrap-or-abort:
=======
for error handling — use the `Must*` helpers to unwrap-or-abort:
>>>>>>> Stashed changes

```pika
use file;
use strings;

export fn main() void = {
	// Write
	let fd = file::MustOpen("/tmp/pika_demo.txt", "w");
	let msg = "hello from pika\n";
	file::MustWrite(fd, strings::toutf8(msg));
	file::Close(fd);

	// Read back
	let fd2 = file::MustOpen("/tmp/pika_demo.txt", "r");
	let buf: [64]u8 = [0...];
	let n = file::MustRead(fd2, buf[0z..64z]);
	file::Close(fd2);

	assert(n == len(msg): i64);

	// Clean up
	file::MustDel("/tmp/pika_demo.txt");
};
```

For explicit error handling, use the unwrapped variants with `match`:

```pika
use file;

fn try_read(path: str) i64 = {
	let result = file::Open(path, "r");
	match (result) {
	case let fd: i32 =>
		let buf: [64]u8 = [0...];
		let n = file::MustRead(fd, buf[0z..64z]);
		file::Close(fd);
		return n;
	case file::FileError =>
		return -1;
	};
};
```

Resources are always explicit: you open a file descriptor, you close it. There
is no RAII, no garbage collector, no defer. The cost of every operation is
visible in the source.

## Safety Guarantees

Pika makes "the safe thing" the default:

- Bounds checks on array/slice indexing
- Compile-time rejection of recursion and function pointers
- Compile-time enforcement of nullable-pointer checks via `match`
- Exhaustive `match` and `switch` (unless you add a default case)
- No variable shadowing

The main escape hatch is unbounded pointers `*[*]T`, used for C interop:

```pika
use strings;

fn prefix(raw: *[*]u8, start: size, end: size) str = {
	let view: []u8 = raw[start..end];
	return strings::fromutf8_unsafe(view);
};

export fn main() void = {
	let buf: [5]u8 = ['P', 'I', 'K', 'A', '!'];
	let raw: *[*]u8 = &buf: *[*]u8;
	assert(prefix(raw, 0, 4) == "PIKA");
};
```

Unbounded pointers are not bounds-checked. Use them at the edges of your
program for C interop, and convert to checked slices as soon as possible. Keep
the unsafe surface area small.
