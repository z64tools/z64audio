ifeq (,$(wildcard settings.mk))
  $(info ExtLib Install)
  $(info Provide path to ExtLib (Either existing or a location to clone to))
  $(info Example: "/home/x/ExtLib/")
  $(info  )
  $(shell ./setup.sh)
  
  include settings.mk
  
  $(shell git clone --recurse-submodules https://github.com/rankaisija64/ExtLib.git $(PATH_EXTLIB))
  $(info  )
  ifeq (,$(wildcard $(PATH_EXTLIB)ExtLib.c))
  	$(shell rm settings.mk)
    $(error Could not locate 'ExtLib.c'... Please try again!)
  endif
  $(info ExtLib Installed successfully!)
endif

include settings.mk