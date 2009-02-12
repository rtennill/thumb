-include Makedefs

TARG= thumb
APP= Thumb.app
#------------------------------------------------------------------------------

$(TARG) :
	$(MAKE) -C src

clean :
	$(MAKE) -C src clean

osxdist : $(TARG)

	mkdir lib

	$(CP) /opt/local/lib/libSDL-1.2.0.dylib    lib
	$(CP) /opt/local/lib/libfreetype.6.dylib   lib
	$(CP) /opt/local/lib/libjpeg.62.dylib      lib
	$(CP) /opt/local/lib/libpng12.0.dylib      lib
	$(CP) /opt/local/lib/libz.1.dylib          lib

	$(CH) /opt/local/lib/libSDL-1.2.0.dylib    \
          @executable_path/lib/libSDL-1.2.0.dylib  $(TARG)
	$(CH) /opt/local/lib/libfreetype.6.dylib   \
          @executable_path/lib/libfreetype.6.dylib $(TARG)
	$(CH) /opt/local/lib/libjpeg.62.dylib      \
          @executable_path/lib/libjpeg.62.dylib    $(TARG)
	$(CH) /opt/local/lib/libpng12.0.dylib      \
          @executable_path/lib/libpng12.0.dylib    $(TARG)
	$(CH) /opt/local/lib/libz.1.dylib          \
          @executable_path/lib/libz.1.dylib        $(TARG)

$(APP) :

	mkdir $(APP)
	mkdir $(APP)/Contents
	mkdir $(APP)/Contents/MacOS
	mkdir $(APP)/Contents/Resources
	mkdir $(APP)/Contents/Frameworks

	$(CH) /opt/local/lib/libSDL-1.2.0.dylib    \
          @executable_path/../Frameworks/libSDL-1.2.0.dylib  $(TARG)
	$(CH) /opt/local/lib/libfreetype.6.dylib   \
          @executable_path/../Frameworks/libfreetype.6.dylib $(TARG)
	$(CH) /opt/local/lib/libjpeg.62.dylib      \
          @executable_path/../Frameworks/libjpeg.62.dylib    $(TARG)
	$(CH) /opt/local/lib/libpng12.0.dylib      \
          @executable_path/../Frameworks/libpng12.0.dylib    $(TARG)
	$(CH) /opt/local/lib/libz.1.dylib          \
          @executable_path/../Frameworks/libz.1.dylib        $(TARG)

	$(CP) $(TARG) $(APP)/Contents/MacOS
	$(CP) app/Info.plist $(APP)/Contents
	$(CP) app/thumb.icns $(APP)/Contents/Resources

	$(CP) /opt/local/lib/libSDL-1.2.0.dylib  $(APP)/Contents/Frameworks
	$(CP) /opt/local/lib/libfreetype.6.dylib $(APP)/Contents/Frameworks
	$(CP) /opt/local/lib/libjpeg.62.dylib    $(APP)/Contents/Frameworks
	$(CP) /opt/local/lib/libpng12.0.dylib    $(APP)/Contents/Frameworks
	$(CP) /opt/local/lib/libz.1.dylib        $(APP)/Contents/Frameworks

#------------------------------------------------------------------------------
