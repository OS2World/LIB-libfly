/* REXX */

   say
   say "libfly installation"
   say
   if RxFuncQuery("SysLoadFuncs") then do
      call RxFuncAdd "SysLoadFuncs","RexxUtil","SysLoadFuncs"
      call SysLoadFuncs
   end

/* guess where EMX is installed */

   emxbind = SysSearchPath('PATH', 'emxbind.exe')
   if emxbind == "" then
   do
       say "Can't find where EMX is installed -- don't know where to install"
       exit
   end
   emxpath = substr(emxbind,1,length(emxbind)-15)
   say "EMX is found in" emxpath
   
/* tell the user where we're going to put things and wait for decision */

   have_dlls = stream("fly_vio.dll", "c", "query exists")
   say "The following files will be installed:"
   say
   say "C header file:   " emxpath"\fly\fly.h"
   say "C header file:   " emxpath"\fly\types.h"
   say "C header file:   " emxpath"\fly\messages.h"
   say "C library file:  " emxpath"lib\fly_os2vio.a"
   say "C library file:  " emxpath"lib\fly_os2pm.a"
   say "C library file:  " emxpath"lib\fly_os2x.a"
   say "C library file:  " emxpath"lib\fly_os2x11.a"
   if have_dlls <> "" then
   do
   say "Runtime DLL:     " emxpath"dll\fly_vio.dll"
   say "Runtime DLL:     " emxpath"dll\fly_pm.dll"
   say "Runtime DLL:     " emxpath"dll\fly_x.dll"
   say "Runtime DLL:     " emxpath"dll\fly_x11.dll"
   end
   say

/* copy files */

   "@mkdir" emxpath"include\fly"
   
   emxpath = translate(emxpath, "/", "\")
   say emxpath"include/fly/fly.h";      "@cp fly/fly.h" emxpath"include/fly/fly.h"
   say emxpath"include/fly/types.h";    "@cp fly/types.h" emxpath"include/fly/types.h"
   say emxpath"include/fly/messages.h"; "@cp fly/messages.h" emxpath"include/fly/messages.h"
   say emxpath"lib/fly_os2vio.a";        "@cp libfly_os2vio.a" emxpath"lib/fly_os2vio.a"
   say emxpath"lib/fly_os2pm.a";         "@cp libfly_os2pm.a" emxpath"lib/fly_os2pm.a"
   say emxpath"lib/fly_os2x.a";          "@cp libfly_os2x.a" emxpath"lib/fly_os2x.a"
   say emxpath"lib/fly_os2x11.a";        "@cp libfly_os2x11.a" emxpath"lib/fly_os2x11.a"
   if have_dlls <> "" then
   do
   say emxpath"dll/fly_vio.dll";   "@cp fly_vio.dll" emxpath"dll/fly_vio.dll"
   say emxpath"dll/fly_pm.dll";    "@cp fly_pm.dll" emxpath"dll/fly_pm.dll"
   say emxpath"dll/fly_x.dll";     "@cp fly_x.dll" emxpath"dll/fly_x.dll"
   say emxpath"dll/fly_x11.dll";   "@cp fly_x11.dll" emxpath"dll/fly_x11.dll"
   end

   say "Installation complete"
