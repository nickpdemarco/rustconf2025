# Rust @ Adobe - Zngur Investigations

## Intro

- For the past few months I've been contributing to Zngur to get it production ready for Adobe's needs.
- I'll give us a quick and incomplete summary of Adobe's needs, how Zngur meets them, what I needed to add, and what's to come.
- Keep in mind that this work is still in development - as I write this we're rolling out a beta internally, but more work is scheduled.

## Zngur, at a high level

### [Example 1](/example1)

```rust
mod generated;

pub struct Foo {
  value: i32
}

pub fn get_a_foo() -> Foo {
  Foo { value: 42 }
}
```

```
// main.zng
mod crate {

  type Foo {
    #layout(size = 4, align = 4);
    field value (offset = 0, type = i32);
  }

  fn get_a_foo() -> Foo;
}
```

```cpp
#include <iostream>
#include "./myrustcode/generated.h"

int main() {
    std::cout << "The value is "
              << int32_t(rust::crate::get_a_foo().value)
              << '\n';
}
```

### [Example 2](/example2)

```rust
mod generated;
```

```
// main.zng
mod ::std {

  mod vec {

    type Vec<i32> {
        #layout(size = 24, align = 8);

        fn new() -> Vec<i32>;
        fn push(&mut self, i32);
        fn get(&self, usize)
            -> ::std::option::Option<&i32> deref [i32];
    }

  }

  type option::Option<&i32> {
      #layout(size = 8, align = 8);
      fn unwrap(self) -> &i32;
  }

}
```

```cpp
#include <iostream>
#include "./myrustcode/generated.h"

int main() {
    auto v = rust::std::vec::Vec<int32_t>::new_();
    v.push(42);
    std::cout << "The value is "
              << *v.get(0).unwrap()
              << '\n';
}
```

## Omnibus / Rust Unified Library

We notice two properties of Rust artifacts:

1. Statically linking multiple Rust-built `.a` files is [unsupported](https://github.com/rust-lang/rust/issues/44283#issuecomment-328181342).
2. Rust crates can bundle arbitrary C libraries.

Property 1 gives us (at least) two options for integrating Rust and C++
- Isolate each Rust library in a DLL, each with a stable C API.
- Synthesize a single Rust library, which depends on all Rust libraries in your build tree, and statlically link it into your executable.

Property 2 gives us (apparently) one option:
- Any Rust module linked into a C++ application must be behind a DLL boundary.

Taken together, we believe the most reasonable path forward is
- Synthesize a single Rust library, which depends on all Rust libraries in your build tree, and *dynamically* link it into your executable.

This ensures
- we don't get ODR violations on unmangled C symbols,
- we may maintain disjoint Rust crates in a single build tree
- we minimize the binary-size overhead of duplicate Rust std libraries

Notably, Firefox uses a very similar approach they call a ["Rust unified library"](https://firefox-source-docs.mozilla.org/build/buildsystem/rust.html). We call it an omnibus.

## `import ".zng"`

Effectively using `zngur` in our build system seemed to require the ability to merge many `.zng` files together.
- Independent crates in our build might require different APIs on `::std` components.
- We need to merge those requirements before running `zngur generate`.
- Otherwise, we'd get multiple-definition errors on the C++-side FFI glue.

[Zngur PR #43](https://github.com/HKalbasi/zngur/pull/43) added this support.
Zngur now has an intuitive merge strategy that computes the partial union of all the .zng declarations,
failing on direct contradictions that cannot be resoloved (e.g. conflicting layout directives).
"Intuitive" is a design goal - the semantics of "merge" should be totally unsurprising to users,
as if they had written all API requirements in the same `.zng` file.

## Next steps

### [Zngur issue 53](https://github.com/HKalbasi/zngur/issues/53)
We need a strategy for using Zngur in our host application *and* upstream dependencies we do not control. If both use zngur, we need to ensure that Zngur glue code cannot cause ODR violations.

- Still early in the process here, but we expect a `#namespace "Adobe"` directive in "top-level" `.zng` files will be sufficient.
- This would instruct zngur to emit symbols of the form `__zngur_Adobe_std_fmt_etc` to reduce the risk of ODR violations.
- We may also embed a hash (the provenance of which is unclear) to prevent identical namespaces from causing ODR violations.

### [Zngur issue 48](https://github.com/HKalbasi/zngur/issues/48)
We need the emitted `generated.h` header to be split up along some reasonable semantic lines, primarily to improve incremental compilation performance.
Note: A 60-line Zngur spec of ours creates a 4,000-line `generated.h`. This won't scale.

- Still in design. Approximate plan: emit one header per crate. But, where would this symbol go?

```
mod ::std {
    type option::Option<::some_crate::SomeType> { // where should this go? std.h or some_crate.h?
        #layout(size = 8, align = 4);
        fn unwrap(self) -> i32;
    }
}
```

In this example, some_crate depends on std, but if we want to put Option in std,
we need to have SomeType and so some_crate, which is circular.

Please add to the discussion in #48 if you have ideas, or share them here.
