# Changelog

All notable changes to GNU recutils will be documented in this file.

For historical release notes prior to this changelog, see the [NEWS](NEWS) file.

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
