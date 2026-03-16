import random
def round_function (r,k):
  return r^ k # Simple XOR with the key
def feistel_decrypt (plain_text, keys, rounds=4):
  L, R = plain_text >> 12, plain_text & 0xFFF
  for i in reversed(range(rounds)):
    L, R = R, L ^ round_function(R, keys[i])
    print(f"Round {i}: L={hex(L)}, R={hex(R)}")

  return (L<< 12) | R

def feistel_encrypt(cipher_text, keys, rounds=4) :
  L, R = cipher_text >> 12, cipher_text & 0xFFF
  for i in (range(rounds)):
      L, R = R ^ round_function(L, keys[i]), L
      print(f"Round {i}: L={hex(L)}, R={hex(R)}")
  return (L<< 12) |R

# Example usage
keys = [0xE, 0xEE, 0xEEE, 0xaaa] # Example keys for 4 rounds
for i in range(5):
  plain_text = i*64 # Example 16-bit input
  cipher_text = feistel_encrypt(plain_text >> 6, keys)
  decrypted_text = feistel_decrypt(cipher_text, keys)
  print (f"Original: {hex(plain_text)}")
  print (f"Encrypted: {hex(cipher_text)}")
  print (f"Decrypted: {hex((decrypted_text << 6) | (plain_text & 0x3F))}")

# Round 0: L=0xe, R=0x0
# Round 1: L=0xe0, R=0xe
# Round 2: L=0xe00, R=0xe0
# Round 3: L=0x44a, R=0xe00
# Round 3: L=0xe00, R=0xe0
# Round 2: L=0xe0, R=0xe
# Round 1: L=0xe, R=0x0
# Round 0: L=0x0, R=0x0
# Original: 0x0
# Encrypted: 0x44ae00
# Decrypted: 0x0
# Round 0: L=0xf, R=0x0
# Round 1: L=0xe1, R=0xf
# Round 2: L=0xe00, R=0xe1
# Round 3: L=0x44b, R=0xe00
# Round 3: L=0xe00, R=0xe1
# Round 2: L=0xe1, R=0xf
# Round 1: L=0xf, R=0x0
# Round 0: L=0x0, R=0x1
# Original: 0x40
# Encrypted: 0x44be00
# Decrypted: 0x40
# Round 0: L=0xc, R=0x0
# Round 1: L=0xe2, R=0xc
# Round 2: L=0xe00, R=0xe2
# Round 3: L=0x448, R=0xe00
# Round 3: L=0xe00, R=0xe2
# Round 2: L=0xe2, R=0xc
# Round 1: L=0xc, R=0x0
# Round 0: L=0x0, R=0x2
# Original: 0x80
# Encrypted: 0x448e00
# Decrypted: 0x80
# Round 0: L=0xd, R=0x0
# Round 1: L=0xe3, R=0xd
# Round 2: L=0xe00, R=0xe3
# Round 3: L=0x449, R=0xe00
# Round 3: L=0xe00, R=0xe3
# Round 2: L=0xe3, R=0xd
# Round 1: L=0xd, R=0x0
# Round 0: L=0x0, R=0x3
# Original: 0xc0
# Encrypted: 0x449e00
# Decrypted: 0xc0
# Round 0: L=0xa, R=0x0
# Round 1: L=0xe4, R=0xa
# Round 2: L=0xe00, R=0xe4
# Round 3: L=0x44e, R=0xe00
# Round 3: L=0xe00, R=0xe4
# Round 2: L=0xe4, R=0xa
# Round 1: L=0xa, R=0x0
# Round 0: L=0x0, R=0x4
# Original: 0x100
# Encrypted: 0x44ee00
# Decrypted: 0x100