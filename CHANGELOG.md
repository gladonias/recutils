# Changelog

All notable changes to GNU recutils will be documented in this file.

For historical release notes prior to this changelog, see the [NEWS](NEWS) file.

## [1.9.3] - 2026-04-27

### Security

- Fix memory leak in `recutl_yesno`: the string returned by `readline()`
  was never freed, leaking memory on every call and on every invalid
  input retry.
  (recutl.c)

- Fix NULL pointer dereference after `fdopen()` in
  `recutl_write_db_to_file()` and in `rec_int_check_rset()` (under
  `REMOTE_DESCRIPTORS`).  If `fdopen` failed, the NULL `FILE *` was
  passed directly to `rec_writer_new` / `curl_easy_setopt`, causing a
  crash.
  (recutl.c, rec-int.c)

- Fix string ownership confusion in selection expression evaluator:
  `rec_sex_eval_node()` returned `str_val` with mixed ownership
  (heap-allocated, borrowed from AST, or static `""`), making it unsafe
  to free results and causing memory leaks in all callers.  All string
  paths now return heap-allocated strings, and a cleanup label frees
  intermediate child values on all exit paths.
  (rec-sex.c)

- Fix unchecked floating-point division by zero in the selection
  expression evaluator.  The integer division path checked for zero
  divisor, but the floating-point path did not, producing silent NaN/Inf.
  (rec-sex.c)

- Fix missing NULL check after `malloc()` in `rec_record_uniq()`.
  On out-of-memory, `memset` and subsequent array access would crash.
  (rec-record.c)

- Fix unsafe `realloc()` in `rec_buf_close()` where failure would lose
  the original buffer pointer (memory leak) and then crash on
  NUL-termination.
  (rec-buf.c)

## [1.9.2] - 2026-04-27

### Added

- **macOS support**: recutils now builds and passes all tests on macOS
  (both Apple Silicon and Intel) without requiring `--disable-*`
  workarounds.

- **Native macOS UUID support**: `configure` now detects macOS's built-in
  `uuid_generate` (from `<uuid/uuid.h>`) when `libuuid` is not present.
  UUID field types and auto-generation work natively on macOS without
  installing any additional packages.

- **Bison version check**: `configure` now fails immediately with an
  actionable error message when GNU Bison >= 3.6 is not found, instead
  of silently falling back and producing a cryptic build failure later.

- **Texinfo check**: `configure` now requires `makeinfo` (GNU Texinfo)
  and fails early with install instructions if it is missing, instead of
  failing midway through `make`.

- **MDB default on macOS**: `mdb2rec` (which depends on `libmdb`) is now
  disabled by default on macOS (`darwin*`), since `libmdb` is not
  readily available. It remains enabled by default on Linux. Users can
  override with `--enable-mdb`.

- **Bash header auto-detection**: The build system now auto-detects bash
  development headers across `/usr/include/bash` (Linux),
  `/opt/homebrew/opt/bash/include/bash` (macOS Apple Silicon), and
  `/usr/local/opt/bash/include/bash` (macOS Intel), instead of
  hardcoding `/usr/include/bash`. Manual override via
  `--with-bash-headers=DIR` is still supported.

- **macOS install instructions** added to README.md with Homebrew
  dependencies and PATH configuration for keg-only formulas.

## [1.9.1] - 2026-04-26

### Security

- Fix buffer overflow in selection expression index parser
  (`rec_sex_lex_extract_index`). A crafted expression with an overlong
  bracket index could overflow a stack buffer.
  (rec-sex-lex.l)

- Fix NULL pointer dereference in `rec_field_set_name` and
  `rec_field_set_value` when called with NULL arguments.
  [CVE-2019-6460](https://nvd.nist.gov/vuln/detail/CVE-2019-6460),
  [Savannah #55556](https://savannah.gnu.org/bugs/?55556)

- Fix NULL pointer dereference in `rec_fex_size` and `rec_fex_get`
  when called with a NULL fex argument.
  [CVE-2019-6456](https://nvd.nist.gov/vuln/detail/CVE-2019-6456),
  [Savannah #55551](https://savannah.gnu.org/bugs/?55551)

- Harden `rec_mset_elem_destroy` against double-free by nullifying
  element data after disposal.
  [CVE-2019-6455](https://nvd.nist.gov/vuln/detail/CVE-2019-6455),
  [Savannah #55550](https://savannah.gnu.org/bugs/?55550)

- Fix multiple NULL pointer dereferences after failed `malloc` in
  `rec_encrypt` and `rec_decrypt`. Add NULL checks for all allocation
  results in the encryption/decryption paths.
  (rec-crypt.c)

- Fix memory leaks in encryption/decryption functions:
  - `rec_encrypt`: `real_in` buffer leaked on all error paths and on
    success.
  - `rec_decrypt`: `*out` buffer leaked on CRC mismatch and decryption
    error paths.
  - `rec_decrypt_field`: `decrypted_value` leaked after setting field
    value.
  - `rec_encrypt_field`: `field_value` leaked when field is already
    encrypted or encryption fails.
  (rec-crypt.c)

- Fix buffer over-read in `rec_encrypt`: `memcpy` copied `in_size + 4`
  bytes from input buffer of only `in_size` bytes.
  (rec-crypt.c)

- Remove debug `printf("error setting key\n")` left in `rec_decrypt`.
  (rec-crypt.c)

- Fix memory leak of `effective_field_name` in selection expression
  evaluator when looking up subfields (e.g. `Field.Subfield`).
  (rec-sex.c)

- Fix NULL pointer dereference in string concatenation operator when
  `malloc` fails.
  (rec-sex.c)

- Fix integer overflow in `rec_db_add_random_indexes`: the computation
  `sizeof(size_t) * ((num+1)*2)` could wrap, leading to undersized
  heap allocation and subsequent buffer overflow.
  (rec-db.c)

- Fix memory leak in `rec_db_new`: when aggregate registry allocation
  failed, the already-created `rset_list` was leaked. Also fix NULL
  pointer dereference when `rset_list` allocation fails (code fell
  through to access freed pointer).
  [CVE-2019-6457](https://nvd.nist.gov/vuln/detail/CVE-2019-6457),
  [Savannah #55553](https://savannah.gnu.org/bugs/?55553)

- Fix buffer leaks on out-of-memory in `rec_parse_field_value`,
  `rec_parse_field_name`, and `rec_parse_comment`: early returns
  bypassed `rec_buf_close`, leaking the buffer struct and its data.
  [CVE-2019-6458](https://nvd.nist.gov/vuln/detail/CVE-2019-6458),
  [Savannah #55554](https://savannah.gnu.org/bugs/?55554)

- Fix memory leak of `rec_type` in external descriptor handling: the
  string returned by `rec_extract_type` was never freed.
  [CVE-2019-6459](https://nvd.nist.gov/vuln/detail/CVE-2019-6459),
  [Savannah #55555](https://savannah.gnu.org/bugs/?55555)

### Fixed

- Fix build failure with pkg-config-based curl detection: use
  `$(CURL_LIBS)` instead of `$(CURLLIBS)` in src/Makefile.am, matching
  the variable name set by `PKG_CHECK_MODULES`.

- Fix multi-line comments being flattened into a single line when
  writing back through `recset`. The parser stored `#` instead of `\n`
  for inter-line separators because `ci` (the `#` codepoint) was
  passed to `rec_buf_putc` instead of the reassigned newline value.
  [Savannah #67856](https://savannah.gnu.org/bugs/?67856)

- Fix memory leak of `subname` in selection expression lexer when
  parsing dotted field names (e.g. `Field.Subfield`).
  (rec-sex-lex.l)

- Fix logic error in `rec_sex_lex_extract_index` where `rec_atoi`
  failure was silently ignored (result overwritten by unconditional
  `res = true`).
  (rec-sex-lex.l)

- Fix `recsel` failing to match multi-value fields when `#Field`
  co-occurs with `Field` in a selection expression (e.g.
  `#Field > 1 && Field = 'x'`). The evaluator now uses AST node
  fixing to bind each field value in turn while preserving the
  original record for `#Field` counting.
  [Savannah #62992](https://savannah.gnu.org/bugs/?62992)

- Fix signed/unsigned comparison bug in `rec_buf_rewind`: the guard
  `(buf->used - n) >= 0` was always true for `size_t`, allowing
  underflow on negative `n`.
  (rec-buf.c)

- Replace `exit(EXIT_FAILURE)` calls in library code with proper
  error returns. A shared library must never terminate the calling
  process. Affected: `rec_sex_eval_node` (REC_SEX_NOVAL case) and
  `rec_type_new` (REC_TYPE_NONE case).
  (rec-sex.c, rec-types.c)

- Add NULL-check error propagation for `strdup` return values in
  `rec_sex_ast_node_set_str`, `rec_sex_ast_node_set_name`,
  `rec_sex_ast_node_fix`, and `rec_sex_ast_fix_name`. These functions
  now return `bool` and propagate allocation failures to callers.
  Callers in the lexer and evaluator updated to handle failures.
  (rec-sex-ast.c, rec-sex-ast.h, rec-sex-lex.l, rec-sex.c)

- Fix `rec_comment_set_text` to check `strdup` return and avoid
  freeing the original comment on allocation failure. Now returns
  `bool`.
  (rec-comment.c, rec.h)

- Fix memory leak in `rec_write_comment`: early returns inside the
  `strsep` loop skipped `free(orig_str)`. Also added NULL check for
  the initial `strdup`.
  (rec-writer.c)

- Include the invalid type descriptor string in `recfix` error
  messages for `%type` and `%typedef` validation, so users can
  identify which descriptor is malformed.
  [Savannah #30674](https://savannah.gnu.org/bugs/?30674)

- Fix memory leak in `recutl_build_db`: use `rec_db_destroy` instead
  of `free` so internal structures (rset list, aggregate registry) are
  properly released on parse failure.
  (recutl.c)

- Fix NULL pointer dereference in `rec_db_delete` when the requested
  record type does not exist. Added missing NULL guard matching the
  pattern already present in `rec_db_set`.
  (rec-db.c)

- Fix `recsel -c` printing record count with `%d` instead of `%zu`,
  which truncated or misrepresented counts on 64-bit systems.
  (recsel.c)

- Fix potential out-of-bounds access in `rec2csv` when a field name
  is an empty string: `field_name[strlen(field_name)-1]` would read
  before the buffer.
  (rec2csv.c)

- Fix memory leaks in `rec_db_merge_records`: all OOM error paths now
  properly clean up the partially-built merge record, duplicated
  fields, and the iterator.
  (rec-db.c)

- Fix memory leaks in `rec_db_join`: OOM errors during the join loop
  and descriptor construction now clean up the join rset and all
  intermediate allocations.
  (rec-db.c)

- Fix unsafe `realloc` pattern in `csv2rec`: the result was assigned
  directly to `ctx->field_names`, losing the original pointer on
  failure. Now uses a temporary variable and aborts on OOM.
  (csv2rec.c)

- Check `asprintf` return values in aggregate functions (`Count`,
  `Avg`, `Min`, `Max`, `Sum`), selection expression evaluator
  (`rec_sex_eval`), and record location formatting. Failed calls
  now produce NULL instead of an indeterminate pointer.
  (rec-aggregate.c, rec-sex.c, rec-record.c)

- Fix extra `printf` argument in `recutl_print_help_footer`: format
  string had one `%s` but two arguments were passed.
  (recutl.c)

- Replace `size_t num_rec = -1` (unsigned wraparound from SIZE_MAX)
  with `size_t num_rec = 0` and restructured loops in `rec_db_query`,
  `rec_db_insert`, `rec_db_delete`, and `rec_db_set`.
  (rec-db.c)

- Fix signed/unsigned type mismatch: `rec_fex_elem_min`/`max` return
  `int` (with `-1` sentinel) but results were stored in `size_t`.
  Changed local variables to `int` with explicit casts in comparisons.
  (rec-db.c)

- Remove unused variables: `rset_size` (recsel.c), `recutl_sort_p`,
  `recutl_order_rset`, `recutl_order_by_field` (recutl.c), `res`
  (rec-field.c, rec-record.c), `mode` (rec-writer.c), and fix
  `const` qualifier warning in `rec_record_contains_value`.

### Build

- Enable `-Wall` compiler warnings in `src/Makefile.am` and
  `utils/Makefile.am`. All project source files now compile
  warning-free under `-Wall`; remaining warnings are confined to
  generated code (flex/bison) and gnulib interface mismatches.

- Fix incompatible pointer type error in `bash/testrec.c`: change
  `argv` from `const char **` to `char **` to match the return type
  of `make_builtin_argv`, resolving `-Wincompatible-pointer-types`
  and `-Wdiscarded-qualifiers` warnings.

### Added

- `recsel`: new `-J`/`--left-join` option to perform a left outer
  join. Records from the primary record set that have no matching
  foreign key in the joined set are included in the output (with the
  foreign key field removed), unlike `-j` (inner join) which omits
  them.
  [Savannah #59283](https://savannah.gnu.org/bugs/?59283)

- `rec_db_query`: new `REC_F_LEFT_JOIN` flag for the public C API to
  request left outer join semantics through the library.

### Known Issues

- **Weak key derivation in `rec-crypt.c`**: The AES-128 encryption
  uses a naive key derivation (password bytes cycled to fill the key)
  and only 4 of 16 IV bytes are random. This provides inadequate
  key stretching and reduced IV entropy. Fixing this requires a
  versioned ciphertext format with backward-compatible decryption,
  which is deferred to a future release. Users with strong
  confidentiality requirements should use external encryption tools
  (e.g. GPG) rather than relying on the built-in field encryption.
