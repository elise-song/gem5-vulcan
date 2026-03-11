import random

sbox = [0] * 12
for i in range(12):
  sbox[i] = random.getrandbits(24)

pbox = [0,1,2,3,4,5,6,7,8,9,10,11] # bit 0 is mapped to bit 11
random.shuffle(pbox)

print ('[{}]'.format(', '.join(hex(x) for x in sbox)))
print(pbox)

sbox = [0x9d0222, 0x18e7d1, 0x6a3b7e, 0x3f7fdd, 0xc6ff21, 0xe2b187, 0xbda80f, 0x516694, 0x42e5cf, 0xc16ba5, 0x96215b, 0x625894]
pbox = [9, 4, 11, 10, 6, 0, 1, 7, 3, 8, 5, 2]

def substitute(x):
  y = 0
  for s in sbox:
    y_bit = 0
    for i in range(24):
      if ((s >> i) & 1) == 1:
        bit = ((x >> i) & 1)
        # print("bit " + str(bit))
        if i == 0:
          y_bit = bit
        else: 
          y_bit = bit ^ y_bit
        # print("out " + str(out))
    y = (y << 1) | y_bit
    # print(y)
  # output y is 12 bits

  y2 = 0
  for s in sbox:
    bit = 0
    for i in range(24):
      if ((s >> i) & 1):
        bit = bit ^ ((x >> i) & 1)
    y2 = (y2 << 1) | bit
  assert y2 == y

  return y

def permutate(x):
  i = 0
  y = 0
  for p in pbox:
    bit = ((x >> i) & 1) << p
    if i == 0:
      y = bit
    else:
      y = bit | y
    i += 1

  i = 0
  y2 = 0
  # print(bin(x))
  for p in pbox:
    bit = ((x >> i) & 1) << p
    y2 = y2 | bit
    # print(f"{p}th bit is {(x >> i) & 1}")
    i += 1
  assert y2 == y 
  return y
    
def round_function (a,k):
  # concat a and k --> 24 bits
  x = a << 12 | k
  print(hex(x))
  sub = substitute(x)
  print("sub: " + str(hex(sub)))
  per = permutate(sub)
  print("per: " + str(hex(per)))
  return per
 


def feistel_decrypt (plain_text, keys, rounds=4):
  L, R = plain_text >> 12, plain_text & 0xFFF
  print(f"L={hex(L)}, R={hex(R)}")
  for i in reversed(range(rounds)):
    L, R = R, L ^ round_function(R, keys[i])
    print(f"Round {i}: L={hex(L)}, R={hex(R)}")

  return (L<< 12) | R

def feistel_encrypt(cipher_text, keys, rounds=4) :
  L, R = cipher_text >> 12, cipher_text & 0xFFF
  print(f"L={hex(L)}, R={hex(R)}")
  for i in (range(rounds)):
      L, R = R ^ round_function(L, keys[i]), L
      print(f"Round {i}: L={hex(L)}, R={hex(R)}")
  return (L<< 12) |R

keys = [0x325, 0x7AB, 0xC1D, 0x9E2] # Example keys for 4 rounds
victim_accesses = [0,]
for access in victim_accesses:
  plain_text = access # Example 16-bit input
  cipher_text = feistel_encrypt(plain_text >> 6, keys)
  decrypted_text = feistel_decrypt(cipher_text, keys)
  print (f"Original: {hex(plain_text)}")
  print (f"Encrypted: {hex(cipher_text)}")
  print(f"Set: {hex(cipher_text  & 0xff)}")
  print (f"Decrypted: {hex((decrypted_text << 6) | (plain_text & 0x3F))}")

