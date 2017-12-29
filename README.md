use alsa library
`libasound2-dev`

and fftw3
`libfftw3-dev`

Example full required dependencies:
```
$ sudo apt-get install qt4-qmake libqt4-dev libqt4-opengl-dev libqt4-opengl libasound2-dev libfftw3-dev
```

On a `git`+pristine:

```
$ neofetch
MMMMMMMMMMMMMMMMMMMMMMMMMmds+.        @Asus
MMm----::-://////////////oymNMd+`     ----------------
MMd      /++                -sNMd:    OS: Linux Mint 18.3 Sylvia x86_64
MMNso/`  dMM    `.::-. .-::.` .hMN:   Kernel: 4.10.0-42-generic
ddddMMh  dMM   :hNMNMNhNMNMNh: `NMm   Uptime: 2 hours, 48 mins
    NMm  dMM  .NMN/-+MMM+-/NMN` dMM   Packages: 2750
    NMm  dMM  -MMm  `MMM   dMM. dMM   Shell: bash 4.3.48
    NMm  dMM  -MMm  `MMM   dMM. dMM   Resolution: 1280x1024
    NMm  dMM  .mmd  `mmm   yMM. dMM   DE: Cinnamon 3.6.7
    NMm  dMM`  ..`   ...   ydm. dMM   WM: Mutter (Muffin)
    hMM- +MMd/-------...-:sdds  dMM   WM Theme: New-Minty (Mint-Y-Dark-Polo)
    -NMm- :hNMNNNmdddddddddy/`  dMM   Theme: Mint-Y-Dark-Polo [GTK2/3]
     -dMNs-``-::::-------.``    dMM   Icons: Surfn-Numix-Polo [GTK2/3]
      `/dMNmy+/:-------------:/yMMM   Terminal: gnome-terminal
         ./ydNMMMMMMMMMMMMMMMMMMMMM   CPU: Intel Pentium 4 3.20GHz (2) @ 3.200GHz
            .MMMMMMMMMMMMMMMMMMM      GPU: NVIDIA GeForce 8400 GS Rev. 3
                                      Memory: 1653MiB / 2501MiB                                                              
```

Compile with:
```
$ qmake -o Makefile vumeter.pro 
$ make
```
