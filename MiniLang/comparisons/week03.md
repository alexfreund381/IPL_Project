# Week 03: identifier scanning

**Works:** all 10 checks pass in solution mode.

## Comparison with the solution

The logic is the same as the solution's

## The Difference

I wrote `std::string_view` where the solution uses `auto`. The resulting type is the same, because `contents_` is a `string_view`.

## Reflection

Since the two versions behave the same, the comparison was mostly about style. The solution's `auto` is shorter. Writing `std::string_view` out made it clear to me
