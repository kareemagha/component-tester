### Capacitor calculations:

$$\frac{di}{dt} = \frac{25\text{mA}}{2.5\text{ms}} = 10 \text{ A/s}$$

$$\text{Switching frequency } f = \frac{1}{2 \times 2.5\text{ms}} = 200\text{Hz}$$

$$C = I \cdot \frac{\Delta t}{\Delta V}$$

Supply voltage is $5\text{V}$, meaning if we want a maximum supply droop of $5\%$ we find,

$$\Delta V = 0.05 \times 5 = 250\text{mV}$$

$$C = \frac{0.025 \times 2.5 \times 10^{-3}}{0.25} = 250\text{µF}$$

---


### Transistor biasing;

The signal goes low ($0\text{V}$) to turn ON. For a PNP, the junction forward biases when $V_{EB} \geq 0.65\text{V}$,

$$V_{EB} = V_E - V_B = 5.0 - 0 = 5.0\text{V} \gg 0.65\text{V} \quad$$

Base current through $R_B$,

$$I_B = \frac{V_E - V_{BE} - V_{sig}}{R_B} = \frac{5.0 - 0.65 - 0}{4700} \approx 0.92\text{mA}$$

To sink $I_C = 25\text{mA}$, the required forced $\beta$ is,

$$\beta_{forced} = \frac{I_C}{I_B} = \frac{25\text{mA}}{0.92\text{mA}} \approx 27$$

The BC327 datasheet specifies $h_{FE(\text{min})} = 100$ at $I_C = 25\text{mA}$

Since $\beta_{forced} \ll h_{FE(\text{min})}$, the transistor is driven hard into saturation with $V_{CE(sat)} \approx 0.1\text{V}$. The LED current-limiting resistor must then be,

$$R_{LED} = \frac{V_{CC} - V_{CE(sat)} - V_F}{I_C} = \frac{5.0 - 0.1 - 2.0}{0.025} = 116$$