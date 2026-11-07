# Protocol and timing

## Optical signaling

The LED uses on-off keying: light on is logical 1; light off is logical 0. The deployed configuration uses 40 ms per bit. Idle is light on.

Each transmitted byte consists of a low start bit, eight data bits (LSB first), one high stop bit, and two additional high bit periods. Total duration is 12 × 40 ms = 480 ms per byte. The UNO samples the start bit at 0.5T, data bits at 1.5T through 8.5T, and the stop bit at 9.5T, relative to its detected falling edge. These are intended software timing points; sensor response and task scheduling introduce deviations.

The transmitter holds light on for 1.2 seconds before each message. The receiver arms after one second of uninterrupted high input. A newline byte (`0x0A`) terminates the message. It is transmitted optically but is not displayed. Between messages the receiver waits for another steady-light interval. It does not retain framing state across power loss or recover data sent while it was off.

The receiver stores up to 80 printable ASCII bytes plus a null terminator. After reception, it draws the message on the OLED in a single screen update. The two-second inter-byte timeout is longer than the normal 480 ms frame duration. An invalid start, stop, character or length abandons the partial message and returns to synchronization search.

This is simple software framing. There is no length field, escaping, sequence number, CRC, acknowledgement or automatic retry. A wrong but printable byte may pass the available checks.

## Timing calculation

For N text characters:

`nominal time = 1.2 + (N + 1) × 12 × 0.040 seconds`

The additional byte is the newline. For `HELLO`, the result is approximately 4.08 seconds. For 80 characters it is 40.08 seconds. Actual elapsed time can be longer due to network and software scheduling. The rate of useful payload during continuous framed-byte transmission is about 2.08 bytes/second, before the per-message overhead.

## Receiver threshold

`analogRead(A2) > 90` is interpreted as light on. The input divider is LDR to 5V and adjustable resistance to GND. The input voltage increases with received illumination. The threshold and potentiometer setting were selected for the initial physical setup; they require adjustment if the lighting, alignment or components change. No lux calibration is claimed.

## Local HTTP interface

| Endpoint | Method | Function |
| --- | --- | --- |
| `/` | GET | Embedded Arabic phone interface |
| `/send` | POST | Form-encoded `text`, 1–80 printable ASCII characters; whitespace-only text is rejected |
| `/status` | GET | JSON transmitter status |

`POST /send` returns HTTP 202 with `{"id":1}` when queued. HTTP 400 indicates invalid input, 409 indicates another transmission is active, and 503 indicates queue failure. Requests are not automatically retried by the page. Status fields are `busy`, `id` and `completed`; identifiers restart when the ESP32 reboots. Completion indicates that optical transmission finished, not that the receiver accepted it.

A separate FreeRTOS task sends the optical message, while the main Arduino loop handles HTTP requests. Only one message is accepted at a time. The supplied transmitter targets the classic dual-core ESP32 DevKit and pins its transmitter task to core 1.
