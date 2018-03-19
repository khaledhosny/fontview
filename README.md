Font View
=========

![Screenshot](screenshot.png)

A simple font viewer.


Building
--------

Requires Gtk+, Cairo, Freetype, HarfBuzz.

To build:

    $ meson build
    $ ninja -C build
    $ ninja -C build install

Rnning
------

    $ fontview /path/to/a/typeface

See COPYING for license information.
