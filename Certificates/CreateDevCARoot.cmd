makecert.exe -n "CN=DevCARoot,OU=Development,O=Edwig Huisman,C=Netherlands" -r -pe -a sha512 -len 4096 -cy authority -sv DevCARoot.pvk DevCARoot.cer
pvk2pfx.exe -pvk DevCARoot.pvk -spc DevCARoot.cer -pfx DevCARoot.pfx -po DevelopmentRoot