This guide will explain what you need to know in order to test DOOM running on mimiker.

* Within `user` directory, place [ported DOOM sources](https://github.com/rafalcieslak/headless_doom). These sources vary slightly from the original source - in particular, they write to vga memory using `/dev/vga/*`, and the path to doom resource files is corrected to match our current filesystem layout.

```
git clone https://github.com/rafalcieslak/headless_doom user/headless_doom
```

* Place your `doom.wad` inside `./user/headless_doom/`. This has to be the `doom.wad` from *The Ultimate Doom 1.10*, no other IWADs are supported. The `Makefile` for headless_doom will check the md5 sum of the wad file. Note that the file name is case-sensitive. [*If you don't own Ultimate DOOM, you can get it on [Steam](http://store.steampowered.com/app/2280/) or [GOG](https://www.gog.com/game/the_ultimate_doom). It frequently goes on sale for under 3€. If you are a mimiker developer, you can find `doom.wad` on the mimiker dev server.*]

```
cp /path/to/your/doom/doom.wad user/headless_doom/.
```

* Compile mimiker as you normally would. This will recursively compile DOOM sources, and will embed DOOM resource files into kernel image.

```
make
```

* Launch the kernel. Enable `-g` to show video display. Use `klog-mask` to hide kernel debug mesasges. Pass `doom` as the `init` program.

```
./launch -g init=/bin/doom klog-mask=0
```

NOTE: ATM `/dev/scancode` only works in blocking mode, and therefore display only refreshes when you press keys...