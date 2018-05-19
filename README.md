# AnySet
[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/) [![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## Documentation
The documentation for AnySet can be found [here](https://tvanslyke.github.io/AnySetDocs/).
* [te::AnySet Documentation](https://tvanslyke.github.io/AnySetDocs/structte_1_1AnySet.html)
* [te::AnyValue Documentation](https://tvanslyke.github.io/AnySetDocs/structte_1_1AnyValue.html)

### [Tutorials](doc/tutorial/README.md)

## Installation
AnySet can be installed via the standard git-clone -> CMake -> make -> install procedure:
```sh
$ git clone https://github.com/tvanslyke/AnySet.git
$ cd AnySet
$ cmake [options] ./
$ make
$ make install
```

### Prerequisites
Installing AnySet requires only CMake and git.  *Using* AnySet in a C++ project requires a C++17-compliant compiler (see [Supported Compilers and Standard Libraries](#supported-compilers-and-standard-libraries)).

### Running the Tests
The tests can be built from the top-level project directory as follows:
```sh
$ cmake ./
$ make test-anyset
```

To run the tests do:
```sh
$ ./test/test-anyset
```

### Building the Docs
For the latest version of the documentation, see the [Documentation section](#documentation).

AnySet uses Doxygen for its documentation.  The docs can be built from the top-level project directory as follows:
```sh
$ cmake ./
$ make docs
```

To view the docs, open the `doc/html/index.html` in your favorite web browser, do:
```sh
$ <my-favorite-browser> doc/html/index.html
```

## Supported Compilers and Standard Libraries
The test suite compiles and passes with the following compilers:
* clang++-5.0 with libstdc++-8 and libc++-7
* clang++-6.0 with libstdc++-8 and libc++-7
* g++-7.3 with libstdc++-7
* g++-8.1 with libstdc++-8

Other versions or combinations of any of the above compilers and STL implementations may work as well, but have not been tested.

### Contributing
Pull requests and bug reports are welcome!

### Authors
* **Timothy VanSlyke** - vanslyke.t@husky.neu.edu

### License
This project is licensed under the MIT License.
