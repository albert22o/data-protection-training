a = int(input("enter a: "))
b = int(input("enter b: "))
U = [a,1,0]
V = [b,0,1]
while V[0] != 0:
    q = U[0] // V[0]
    T = [U[0] % V[0],U[1] - q* V[1],U[2]- q*V[2]]
    U = V
    V = T
print(U)