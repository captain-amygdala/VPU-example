.equ GPIO, 2
.equ GPSEL, 0x7E200000 + (GPIO/10)*4
.equ GPSET, 0x7E20001C + (GPIO>>5)*4
.equ GPCLR, 0x7E200028 + (GPIO>>5)*4


  mov r1, GPSEL

  ld r0, (r1)
  and r0, ~(0b111<<((GPIO%10)*3)) #set our GPIO as output
  or r0, 1<<((GPIO%10)*3)
  st r0, (r1)

  mov r1, GPSET
  mov r2, GPCLR
  mov r3, 1<<(GPIO%32) #our GPIO's bit

  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  st r3, (r2) #low
  st r3, (r1) #high
  rts
