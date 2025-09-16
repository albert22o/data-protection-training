base = int(input("enter base: "))
power = int(input("enter power: "))
mod = int(input("enter mod: "))
res = 1
s = base
while (power != 0):
    if power % 2 == 1:
        res = (res * s) % mod
    s = (s * s) % mod
    power //= 2
print(res)