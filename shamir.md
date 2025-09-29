# Метод Шамира

# 1. Краткая идея

Алиса (A) и Боб (B) хотят обменяться или передать значение `m` так, чтобы никто посередине не мог узнать `m`. Они работают в группе $({Z}_p^*)$ — целые числа (1..p-1) по умножению по модулю простого (p).

Каждая сторона выбирает пару чисел (c, d) таких, что

$c \cdot d \equiv 1 \pmod{p-1}$.

Тогда последовательность операций даёт восстановление (m) без раскрытия секретов:

1. A: $(x_1 = m^{c_A} \bmod p)$ → B
2. B: $(x_2 = x_1^{c_B} = m^{c_A c_B} \bmod p)$ → A
3. A: $(x_3 = x_2^{d_A} = m^{c_B} \bmod p)$ → B
4. B: $(m = x_3^{d_B} = m^{c_B d_B} = m \bmod p)$.

Идея: умножение показателей по модулю (p-1) делает так, что применение обратных степеней восстанавливает исходный показатель 1.

---

# 2. Как протокол отображён в программе

* Код генерирует простое `p` и пары `(cA, dA)` и `(cB, dB)`, где `c * d ≡ 1 (mod p-1)`.
* Файл шифруется по блокам: каждый блок байт преобразуется в число `m < p` и затем к нему последовательно применяются степени `cA`, `cB`, `dA`, получая `x3`. `x3` записывается в выходной файл.
* При расшифровке читается `x3`, применяется `dB` и восстанавливается исходный `m`.

Программа использует файловую симуляцию обмена вместо реального сетевого обмена.

---

# 3. Ключевые фрагменты кода и пояснения

## 3.1 Генерация пары ключей `(c, d)`

```cpp
std::tuple<ll, ll> generate_shamir_keys(ll p)
{
    if (p <= 3) throw std::runtime_error("p must be > 3");
    ull phi = (ull)(p - 1);
    std::uniform_int_distribution<ull> dist(2, phi - 1);

    for (int attempts = 0; attempts < 1000000; ++attempts)
    {
        ull c = dist(global_rng());
        auto eg = egcd((long long)c, (long long)phi);
        ll g = std::get<0>(eg);
        ll x = std::get<1>(eg);
        if (g != 1) continue;         // gcd(c, phi) == 1 обязателен
        ll d = (ll)(x % (ll)phi);
        if (d < 0) d += (ll)phi;
        ull check = (((__int128)c * (ull)d) % phi);
        if (check != 1ULL) continue;
        // дополнительная проверка:
        ll test = 123;
        ll enc = mod_pow(test, (long long)c, p);
        ll dec = mod_pow(enc, d, p);
        if (dec == test) return {(long long)c, d};
    }
    throw std::runtime_error("generate_shamir_keys: cannot find keys");
}
```

**Пояснение:**

* `phi = p - 1` — порядок группы $(\mathbb{Z}_p^*)$.
* Выбираем случайный `c` в `[2, phi-1]`.
* `egcd(c, phi)` — расширенный Евклид: возвращает `g = gcd(c, phi)` и `x,y` такие, что `c*x + phi*y = g`. Если `g != 1`, обратного не существует — берём другой `c`.
* `d = x mod phi` — обратный к `c` по модулю `phi`.
* Проверяем `(c * d) % phi == 1` с помощью `__int128` для избежания переполнения.
* Тестируем `mod_pow` на небольшом числе (защита от ошибок в реализации `mod_pow`/`egcd`).

---

## 3.2 Быстрое возведение в сетпень по модулю

```cpp
long long mod_pow(long long base, long long exp, long long mod)
{
    if (mod <= 0) throw std::invalid_argument("mod_pow: mod must be > 0");
    long long result = 1 % mod;
    long long cur = base % mod;
    if (cur < 0) cur += mod;

    while (exp > 0)
    {
        if (exp & 1)
        {
            __int128 t = (__int128)result * (__int128)cur;
            result = (long long)(t % mod);
        }
        __int128 t2 = (__int128)cur * (__int128)cur;
        cur = (long long)(t2 % mod);
        exp >>= 1;
    }
    return result;
}
```

**Почему важно:**

* Это быстрое возведение в степень: работает за $(O(\log \text{exp}))$ умножений.
* Промежуточные умножения выполняются в `__int128`, чтобы избежать переполнений 64-битных чисел.
* Если `mod_pow` неверна — весь протокол даст неверный результат.

---

## 3.3 Шифрование файла — основной цикл

Формат выходного файла: заголовок и зашифрованные блоки.

**Запись заголовка:**

```cpp
fout.write("SHAM", 4);
fout.put((char)plain_block);
fout.put((char)cipher_block);
write_le64(fout, (ull)p);
write_le64(fout, orig_size);
```

* `plain_block` — сколько байт исходного файла читается в один блок, так чтобы `m < p`.
* `cipher_block` — сколько байт нужно для записи числа `< p`.
* `orig_size` — исходный размер файла, чтобы при расшифровке обрезать лишние нули в последнем блоке.

**Шифрование блоков:**

```cpp
ull m = bytes_to_ull(inbuf);
if (m >= (ull)p) throw std::runtime_error("Message block is too large for prime p");

// Шаги Шамира
ll x1 = mod_pow((long long)m, cA, p);
ll x2 = mod_pow(x1, cB, p);
ll x3 = mod_pow(x2, dA, p);

auto enc_bytes = ull_to_bytes((ull)x3, cipher_block);
fout.write((const char*)enc_bytes.data(), (std::streamsize)cipher_block);
```

* Читаем `plain_block` байт → `m`.
* Применяем три степенных преобразования: `cA`, `cB`, `dA`.
* Записываем `x3` в `cipher_block` байт.

---

## 3.4 Расшифровка

```cpp
ull x3 = bytes_to_ull(inbuf);
ll m_ll = mod_pow((long long)x3, dB, p);
ull m = (ull)m_ll;
auto outb = ull_to_bytes(m, (size_t)plain_block);

ull remain = orig_size - written;
size_t towrite = (size_t)std::min<ull>((ull)plain_block, remain);
fout.write((const char*)outb.data(), (std::streamsize)towrite);
written += towrite;
```

* Читаем `cipher_block` байт → `x3`.
* Применяем `dB` — последний шаг Боба — восстановление `m`.
* Преобразуем `m` в `plain_block` байт и записываем, при необходимости обрезая последний блок по `orig_size`.

---

# 4. Наглядный численный пример (малые числа)

Возьмём маленькое $p$ для ручных вычислений:

* $p = 101$, тогда $phi = p-1 = 100$.
* **Алиса:** $c_A = 7$. Найдём $d_A$ такое, что $7 \cdot d_A \equiv 1 \pmod{100}$. Решение: $d_A = 43$ (потому что $7 \cdot 43 = 301 \equiv 1 \pmod{100}$).
* **Боб:** $c_B = 11$. $d_B = 91$ (потому что $11 \cdot 91 = 1001 \equiv 1 \pmod{100}$).
* Пусть $m = 20$.

Шаги (с помощью `mod_pow`):

1. A: $x_1 = 20^{7} \bmod 101$.
2. B: $x_2 = x_1^{11} \bmod 101$.
3. A: $x_3 = x_2^{43} \bmod 101 = 20^{11} \bmod 101$.
4. B: $m = x_3^{91} \bmod 101 = 20$.

Конкретные остатки можно вычислить с калькулятором или программно — важно следить за порядком степеней, это иллюстрирует идею.

---

# 5. Короткое математическое объяснение

Выполнение степенных преобразований приводит к возведению `m` в степень, равную произведению применённых показателей. Итоговая степень будет эквивалентна $1 \pmod{p-1}$, т.к.:

* $c_A d_A \equiv 1 \pmod{p-1}$
* $c_B d_B \equiv 1 \pmod{p-1}$

Следовательно итоговая степень в цепочке $m^{c_A c_B d_A d_B}$ эквивалентна $(m^1) \pmod{p}$, и результат — исходное `m`.

Формально мы используем свойства порядка мультипликативной группы и теоремы Ферма/Эйлера.

---

# 8. Резюме

* Протокол Шамира — трёхходовой: A→B→A→B, применяются экспоненты, связанные обратными по модулю `p-1`.
* Ключи: пары `(c, d)` такие, что `c * d ≡ 1 (mod p-1)`.
* Реализация: файл разбивается на блоки `m < p`, к ним последовательно применяются `cA`, `cB`, `dA`; при расшифровке применяется `dB`.
* Самая важная реализация: корректный `mod_pow` (с защитой от переполнения) и корректный `egcd`.
