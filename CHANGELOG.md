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

### Fixed

- Fix build failure with pkg-config-based curl detection: use
  `$(CURL_LIBS)` instead of `$(CURLLIBS)` in src/Makefile.am, matching
  the variable name set by `PKG_CHECK_MODULES`.

- Fix memory leak of `subname` in selection expression lexer when
  parsing dotted field names (e.g. `Field.Subfield`).
  (rec-sex-lex.l)

- Fix logic error in `rec_sex_lex_extract_index` where `rec_atoi`
  failure was silently ignored (result overwritten by unconditional
  `res = true`).
  (rec-sex-lex.l)
