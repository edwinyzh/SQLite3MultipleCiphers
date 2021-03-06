# SQLite3MultipleCiphers

The project **SQLite3 Multiple Ciphers** implements an encryption extension for [SQLite](https://www.sqlite.org) with support for multiple ciphers. In the past the encryption extension was bundled with the project [wxSQLite3](https://github.com/utelle/wxsqlite3), which provides a thin SQLite3 database wrapper for [wxWidgets](https://www.wxwidgets.org/).

In the course of time several developers had asked for a stand-alone version of the _wxSQLite3 encryption extension_. Originally it was planned to undertake the separation process already in 2019, but due to personal matters it had to be postponed for several months. However, maybe that wasn't that bad after all, because there were changes to the public SQLite code on Feb 7, 2020: [“Simplify the code by removing the unsupported and undocumented SQLITE_HAS_CODEC compile-time option”](https://www.sqlite.org/src/timeline?c=5a877221ce90e752). These changes took effect with the release of SQLite version 3.32.0 on May 22, 2020. As a consequence, all SQLite encryption extensions out there will not be able to easily support SQLite version 3.32.0 and later.

In late February 2020 work started on a new implementation of a SQLite encryption extension that will be able to support SQLite 3.32.0 and later. The new approach is based on [SQLite's VFS feature](https://www.sqlite.org/vfs.html). This approach has its pros and cons. On the one hand, the code is less closely coupled with SQLite itself; on the other hand, access to SQLite's internal data structures is more complex.

This project is _Work In Progress_. As of end of May 2020, a - still preliminary - code version based on SQLite 3.32.1 is publicly available. Although several teething problems were resolved since February, it is likely that further code modifications and/or reorganizations will occur, before a first regular release can be made. The code was mainly developed under Windows, but was tested under Linux as well. At the moment no major issues are known, but nevertheless the code should not yet be used for production.

## How to participate

**Help in testing and discussing further development will be _highly_ appreciated**. Please use the [issue tracker](https://github.com/utelle/SQLite3MultipleCiphers/issues) to give feedback, report problems, or to discuss ideas.

## Documentation

Documentation of the currently supported cipher schemes and the C and SQL interfaces is provided already on the [SQLite3MultipleCiphers website](https://utelle.github.io/SQLite3MultipleCiphers/).

Documentation on how to build the extension will be added soon. For the time being please consult the documentation of the [wxSQLite3 encryption extension](https://github.com/utelle/wxsqlite3/blob/master/sqlite3secure/readme.md). Please take notice that the prefix `wxsqlite3` in function names was renamed to `sqlite3mc` in the new implementation.
