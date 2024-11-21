import matplotlib.pyplot as plt

start = 1
end = 100
positions = range(start, end)
values = []

f = open('workloads/createdata_N100_D1000_K5_L5_S1_a1_b1_P0_W5_F.txt', 'r')
for row in f:
    values.append(int(row))
    
#plt.plot(positions, values)
plt.scatter(positions, values[start: end], s = 8)
plt.xlabel("Position")
plt.ylabel("Value")
plt.title("Value vs. Position")
plt.suptitle('1 to 100 from D=1,000, K=5, L=5, a=1, b=1, P=4, W=5, F=True')
plt.show()