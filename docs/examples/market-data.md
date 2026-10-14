# Market Data

Real market data feeds modelled with RBE, decoded through frames.

CMake target: `markets`

## Feeds

??? example "aquis"

    ```cpp title="example/markets/aquis.hpp"
    --8<-- "example/markets/aquis.hpp"
    ```

??? example "cboe"

    ```cpp title="example/markets/cboe.hpp"
    --8<-- "example/markets/cboe.hpp"
    ```

??? example "london"

    ```cpp title="example/markets/london.hpp"
    --8<-- "example/markets/london.hpp"
    ```

??? example "nasdaq"

    ```cpp title="example/markets/nasdaq.hpp"
    --8<-- "example/markets/nasdaq.hpp"
    ```

??? example "opra"

    ```cpp title="example/markets/opra.hpp"
    --8<-- "example/markets/opra.hpp"
    ```

## Decoding

```cpp title="example/markets.cpp"
--8<-- "example/markets.cpp"
```
