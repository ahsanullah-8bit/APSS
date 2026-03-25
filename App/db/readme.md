Compile the model files using

```bash
odb -d sqlite -q -s -o "sqlite" --std c++17 --schema-format separate --profile qt -I<libodb_dir>/include -I<qt_install_headers> "odbmodels/event.h"
```

You must use full paths. You can download the odb compiler, libodb from [here](https://codesynthesis.com/download/odb/2.5.0/) and find Qt headers using

```bash
qmake.exe -query QT_INSTALL_HEADERS
```