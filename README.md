# GNU recutils

GNU recutils is a set of tools and libraries to access human-editable,
plain text databases called **recfiles**. The rec format is
human-readable, requires no external server, and works well with
standard Unix tools like `grep`, `sed`, and version control systems.

Homepage: <https://www.gnu.org/software/recutils/>

## Tools

| Command    | Description                                      |
|------------|--------------------------------------------------|
| `recsel`   | Select and print records from a recfile           |
| `recins`   | Insert records into a recfile                     |
| `recdel`   | Delete records from a recfile                     |
| `recset`   | Set (update) field values in a recfile            |
| `recfix`   | Check recfile integrity and fix common problems   |
| `recinf`   | Print information about a recfile's structure     |
| `recfmt`   | Format records using a template                   |
| `csv2rec`  | Convert CSV data to rec format                    |
| `rec2csv`  | Convert rec data to CSV format                    |
| `mdb2rec`  | Convert Microsoft Access databases to rec format  |

## Installation

### Dependencies

**Required:**

- C compiler (GCC or Clang)
- GNU Autotools (autoconf, automake, libtool)
- GNU Gettext
- GNU Texinfo (makeinfo)
- Flex (>= 2.5.35)
- GNU Bison (>= 3.6)
- pkg-config

**Required libraries:**

- `check` -- unit test framework (needed for `make check`)

**Optional libraries (enable additional features):**

- `libgcrypt` and `libgpg-error` -- field-level encryption support
- `libcurl` -- remote record descriptors
- `libuuid` -- UUID field type (on macOS, UUID is provided natively by the system)
- `libmdb` (mdbtools) -- `mdb2rec` utility for Access database conversion
- `help2man` -- man page regeneration
- Bash development headers -- `readrec`/`testrec` bash loadable builtins

On Arch Linux:

```sh
sudo pacman -S base-devel check help2man libgcrypt curl texinfo
```

On Debian/Ubuntu:

```sh
sudo apt install build-essential autoconf automake libtool gettext \
    flex bison pkg-config check libgcrypt20-dev libcurl4-openssl-dev \
    uuid-dev help2man texinfo
```

On Fedora:

```sh
sudo dnf install gcc autoconf automake libtool gettext-devel flex \
    bison pkgconf check-devel libgcrypt-devel libcurl-devel \
    libuuid-devel help2man texinfo
```

On macOS (via [Homebrew](https://brew.sh/)):

```sh
brew install autoconf automake libtool gettext flex bison \
    pkg-config check help2man texinfo
```

Homebrew installs Bison and Texinfo as "keg-only" (not linked into
`/opt/homebrew/bin`) because macOS ships its own outdated versions.
You must add them to your `PATH` before building:

```sh
# Apple Silicon (M1/M2/M3/M4)
export PATH="/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/texinfo/bin:$PATH"

# Intel Mac
export PATH="/usr/local/opt/bison/bin:/usr/local/opt/texinfo/bin:$PATH"
```

**macOS notes:**
- **UUID** is provided natively by the system (`<uuid/uuid.h>`); no
  extra library or package is needed.
- **mdb2rec** is disabled by default on macOS since `libmdb` is not
  readily available. Pass `--enable-mdb` to override if you have it
  installed.
- **Bash builtins** are auto-detected. If you install `bash` via
  Homebrew (`brew install bash`), the headers are found automatically.
  You can also specify a custom location with
  `--with-bash-headers=/path/to/bash/include`.
- **Encryption** requires `libgcrypt`: `brew install libgcrypt`.

### Building from source

If building from a git checkout (not a release tarball), you need to
initialize the gnulib submodule and run the bootstrap script first:

```sh
git submodule update --init
./bootstrap
```

Then configure, build, and test:

```sh
./configure
make
make check        # run the test suite
sudo make install # install to /usr/local by default
```

To install to a custom prefix:

```sh
./configure --prefix=$HOME/.local
make && make install
```

Notable `configure` options:

- `--disable-encryption` -- build without field encryption (drops libgcrypt dependency)
- `--disable-bash-builtins` -- skip building bash loadable builtins
- `--disable-uuid` -- disable UUID type support
- `--disable-mdb` -- disable mdb2rec (default on macOS)
- `--disable-curl` -- disable remote record descriptors
- `--with-bash-headers=DIR` -- specify bash header location for builtins

## Quick start

Create a recfile `books.rec`:

```
%rec: Book
%key: Id
%type: Id int
%mandatory: Title Author

Id: 1
Title: Moby Dick
Author: Herman Melville

Id: 2
Title: Don Quixote
Author: Miguel de Cervantes

Id: 3
Title: The Trial
Author: Franz Kafka
```

Query it:

```sh
# Select all books
recsel books.rec

# Select by expression
recsel -e 'Author ~ "Melville"' books.rec

# Print specific fields
recsel -p Title,Author books.rec

# Count records
recsel -c books.rec

# Select by index
recsel -n 0-1 books.rec
```

Modify it:

```sh
# Insert a record
recins -t Book -f Id -v 4 -f Title -v "1984" -f Author -v "George Orwell" books.rec

# Update a field
recset -e 'Id = 3' -f Author -s "Franz K." books.rec

# Delete a record
recdel -e 'Id = 4' books.rec

# Validate integrity
recfix books.rec
```

For complete documentation, see the [GNU recutils manual](https://www.gnu.org/software/recutils/manual/).

## Known limitations

- **Weak key derivation in field encryption**: The built-in AES-128
  encryption uses a naive key derivation (password bytes cycled to fill
  the key) and only 4 of 16 IV bytes are truly random.  Users with
  strong confidentiality requirements should use external encryption
  (e.g. GPG) rather than relying on the built-in `%confidential`
  mechanism. Fixing this requires a versioned ciphertext format with
  backward-compatible decryption, which is planned for a future release.

- **Selection option documentation is duplicated** across utility man
  pages. This is tracked as
  [Savannah #41603](https://savannah.gnu.org/bugs/?41603).

- **Emacs rec-mode** is maintained as a separate package (available via
  GNU ELPA) and is not included in this repository.

## Fork information

This repository was forked from the official GNU recutils repository on
[Savannah](https://savannah.gnu.org/git/?group=recutils). The fork
point is commit `0e4c96c` (version 1.9, post-release), which
corresponds to the state of the upstream master branch at the time of
forking.

All changes after that commit (version 1.9.1 onward) were implemented
with the assistance of an AI coding assistant
([Claude Code](https://claude.ai/claude-code) by Anthropic). See
[CHANGELOG.md](CHANGELOG.md) for a detailed list of fixes and
improvements.

## License

GNU recutils is free software, released under the
[GNU General Public License v3.0](COPYING) or later.

Copyright (C) 2010-2022 Jose E. Marchesi and Free Software Foundation, Inc.
