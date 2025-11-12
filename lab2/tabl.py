import matplotlib.pyplot as plt

data = [
    (1, 35.8655, 34.7795, 1.03),
    (5, 35.4959, 13.5627, 2.62),
    (10, 35.6822, 11.4549, 3.12),
    (15, 35.9980, 9.6451, 3.73),
    (20, 35.5687, 10.7387, 3.31),
]

N = [row[0] for row in data]
speedup = [row[3] for row in data]
efficiency = [s / n for s, n in zip(speedup, N)]  

# График ускорения
plt.figure(figsize=(10, 5))
plt.plot(N, speedup, 'b-o', linewidth=2, markersize=6, label='Измеренное ускорение $S_N$')
plt.title('Зависимость ускорения $S_N$ от числа потоков $N$')
plt.xlabel('Число потоков (N)')
plt.ylabel('Ускорение $S_N$')
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()
plt.tight_layout()
plt.savefig('speedup.png', dpi=300)
plt.show()

# График эффективности
plt.figure(figsize=(10, 5))
plt.plot(N, efficiency, 'g-s', linewidth=2, markersize=6, label='Эффективность $E_N$')
plt.title('Зависимость эффективности $E_N$ от числа потоков $N$')
plt.xlabel('Число потоков (N)')
plt.ylabel('Эффективность $E_N$')
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()
plt.tight_layout()
plt.savefig('efficiency.png', dpi=300)
plt.show()