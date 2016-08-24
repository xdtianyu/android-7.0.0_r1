This mockito subproject repackages cglib + asm to avoid problems with conflicting versions.

Unfortunately, jarjar was not able to repackage sources 
therefore I'm using this subproject to repackage cglib+asm.
We should NEVER modify cglib/asm - sources are here only for the sake of repackaging.

Source was obtained from https://github.com/mockito/mockito/tree/master/cglib-and-asm and cglib+asm
has been included in the Mockito builds since 2008 in Mockito 1.5.
