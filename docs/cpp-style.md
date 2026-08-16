# C++ Style

Apply these rules to project-owned C++ code. Generated and third-party code is exempt.

## Formatting

Use the following `clang-format` settings as the style reference:

```yaml
BasedOnStyle: LLVM
Language: Cpp
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100
BreakBeforeBraces: Attach
PointerAlignment: Left
ReferenceAlignment: Left
SortIncludes: CaseSensitive
IncludeBlocks: Regroup
NamespaceIndentation: None
AllowShortFunctionsOnASingleLine: Empty
```

## Static Analysis

Use the following `clang-tidy` settings when performing static analysis:

```yaml
Checks: >-
  -*,
  bugprone-*,
  modernize-*,
  performance-*,
  readability-identifier-naming
WarningsAsErrors: ''
HeaderFilterRegex: '^(?!.*(?:SDL2|SDL2_ttf|tinyxml2)).*'
FormatStyle: file
CheckOptions:
  - key: readability-identifier-naming.NamespaceCase
    value: lower_case
  - key: readability-identifier-naming.ClassCase
    value: lower_case
  - key: readability-identifier-naming.StructCase
    value: lower_case
  - key: readability-identifier-naming.EnumCase
    value: lower_case
  - key: readability-identifier-naming.EnumConstantCase
    value: lower_case
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
  - key: readability-identifier-naming.VariableCase
    value: lower_case
  - key: readability-identifier-naming.PrivateMemberSuffix
    value: '_'
```
