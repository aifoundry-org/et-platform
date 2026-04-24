# erbium-examples / testdata

Classic computer-vision test images used by the end-to-end drivers
under `../scripts/`. Each image is loaded, converted to 8-bit
grayscale, and resized to whatever resolution the target kernel
expects (256x256 for `histogram.elf` today).

| File         | Native size | Source URL |
|--------------|-------------|------------|
| baboon.jpg   | 512x512     | https://raw.githubusercontent.com/opencv/opencv/master/samples/data/baboon.jpg |
| peppers.jpg  | 512x480     | https://raw.githubusercontent.com/opencv/opencv/master/samples/data/fruits.jpg |
| aero1.jpg    | 640x480     | https://raw.githubusercontent.com/opencv/opencv/master/samples/data/aero1.jpg |

These are long-standing public test images bundled with OpenCV and
re-hosted on GitHub at the URLs above. If a file goes missing or
you'd like a fresh copy, re-run:

```bash
cd erbium-examples/testdata
curl -LO https://raw.githubusercontent.com/opencv/opencv/master/samples/data/baboon.jpg
# (repeat with the other URLs from the table)
```
