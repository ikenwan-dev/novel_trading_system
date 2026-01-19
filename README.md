Currently this is an event driven backtesting system that takes in historical data and runs a strategy on it. Once completed a equity curve is generated and saved to a csv file.

Future plans include:
- adding compatability with level 3 tick historical data. Which would allow for more accurate backtesting.
    - This would require a the data handler to do some sort of LOB reconstruction and execution handler to perform matching
    - This would allow for a more accurate slippage model
- add limit order support 
- adding a live trading mode (stretch)
