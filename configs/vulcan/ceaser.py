import random


def substitute(x):
  y = 0
  sbox = [0] * 12
  for i in range(12):
    sbox[i] = random.getrandbits(24)
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
  return y

def permutate(x):
  pbox = [0,1,2,3,4,5,6,7,8,9,10,11] # bit 0 is mapped to bit 11
  random.shuffle(pbox)
  i = 0
  y = 0
  for p in pbox:
    bit = ((x >> i) & 1) << p
    if i == 0:
      y = bit
    else:
      y = bit | y
    i += 1
  return y
    
def round_function (a,k):
  # concat a and k --> 24 bits
  x = a << 12 | k
  # print(hex(x))
  y = permutate(substitute(x))
  return y


def feistel_decrypt (plain_text, keys, rounds=4):
  L, R = plain_text >> 12, plain_text & 0xFFF
  # print(f"L={hex(L)}, R={hex(R)}")
  for i in reversed(range(rounds)):
    L, R = R, L ^ round_function(R, keys[i])
    # print(f"Round {i}: L={hex(L)}, R={hex(R)}")

  return (L<< 12) | R

def feistel_encrypt(cipher_text, keys, rounds=4) :
  L, R = cipher_text >> 12, cipher_text & 0xFFF
  # print(f"L={hex(L)}, R={hex(R)}")
  for i in (range(rounds)):
      L, R = R ^ round_function(L, keys[i]), L
      # print(f"Round {i}: L={hex(L)}, R={hex(R)}")
  return (L<< 12) |R

keys = [0x325, 0x7AB, 0xC1D, 0x9E2] # Example keys for 4 rounds
victim_accesses = [0, 64, 128, 192, 256]
for access in victim_accesses:
  plain_text = access # Example 16-bit input
  cipher_text = feistel_encrypt(plain_text >> 6, keys)
  decrypted_text = feistel_decrypt(cipher_text, keys)
  print (f"Original: {hex(plain_text)}")
  print (f"Encrypted: {hex(cipher_text)}")
  print(f"Set: {hex((cipher_text >> 6) & 0xff)}")
  print (f"Decrypted: {hex((decrypted_text << 6) | (plain_text & 0x3F))}")

