echo Touch all files

set stamp=201604242000

for /r %%G in (*.h)        do touch -t %stamp% "%%G"
for /r %%G in (*.cpp)      do touch -t %stamp% "%%G"
for /r %%G in (*.c)        do touch -t %stamp% "%%G"
for /r %%G in (*.rc)       do touch -t %stamp% "%%G"
for /r %%G in (*.sln)      do touch -t %stamp% "%%G"
for /r %%G in (*.vcxproj*) do touch -t %stamp% "%%G"
for /r %%G in (*.user)     do touch -t %stamp% "%%G"
for /r %%G in (*.txt)      do touch -t %stamp% "%%G"
for /r %%G in (*.pdf)      do touch -t %stamp% "%%G"
for /r %%G in (*.js)       do touch -t %stamp% "%%G"
for /r %%G in (*.docx)     do touch -t %stamp% "%%G"
for /r %%G in (*.vsd)      do touch -t %stamp% "%%G"
for /r %%G in (*.png)      do touch -t %stamp% "%%G"
for /r %%G in (*.exe)      do touch -t %stamp% "%%G"

echo Ready

pause
