import numpy as np
import matplotlib.pyplot as plt

## bike modelling

P = 150 # W
l = 1000 # m
t = 1000/5 # s
m = 70 #kg
g = 9.81 #m / s ** 2
N = 100
min_slope = -0.25
max_slope = 0.25

c2 = m*g
s_bar = np.linspace(min_slope, max_slope, N)
t_scaled = np.empty(N)
for i in range(N):
    for root in np.roots([P*(t**3)/l, -s_bar[i]*c2*(t**2), 0, -P * t ** 3 / l]):
        if np.isreal(root):
            t_scaled[i] = root


# export the function of scaled time to the lua file
print("list = {", end='')
for t_elem in t_scaled:
    print(t_elem, end='')
    if t_elem != t_scaled[-1]:
        print(",", end='')
print("}")
print("N =", N)
print("min_slope =", min_slope)
print("max_slope =", max_slope)

# see a plot with the generated function of scaled time.

t_simple = 1 / (1 - s_bar * 5.0)

plt.plot(s_bar, t_scaled, s_bar, t_simple)
plt.ylim([-1, 6])
plt.title("How much longer a bicycle takes when going uphill")
plt.xlabel('$slope [d_{vertical}/d_{horizontal}]$')
plt.ylabel('$time [t_{withelevation} / t_{flat}]$')
plt.legend(['$t_{proper}$', '$t_{simple}$'])
plt.savefig('bike_slope.png')
plt.show()