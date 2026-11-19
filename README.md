# Camper tanks

The tanks of a camper on a Tessera screen: fresh water, grey water and black water as a tank, gas as a bottle, diesel
as a canister, each filled to its level with the percentage, its name and what is in it (litres, or kilograms of gas).
Red when fresh water, gas or diesel run low, or grey and black water are nearly full.

A tile shows one tank per cell: a 1 x 1 tile one tank, a 2 x 1 or 1 x 2 tile two, a 2 x 2 or 4 x 1 tile four. You choose
which tanks, in which order, for each tile.

It reads the sensors straight from Home Assistant, so it works with any level sensor in percent, such as
[camper-waterlevel](https://github.com/hflocki/camper-waterlevel) (Votronic tank sender, MQTT) or a gas bottle sensor.

## Set up

1. In Tessera, open **Plugins**, **Add with a link**, and paste the link of this repository. Add the plugin to a screen.
2. In the plugin's details on the screen's Plugins tab, under **When building**, choose up to four level sensors in %
   (**Tank 1: level** to **Tank 4: level**) and, if you have them, their content in litres or kilograms
   (**Tank 1: content** ...). Press **Save and build**: the screen builds once.
3. Under **Settings** in the same place, set each tank's kind (fresh water, grey water, black water, gas or diesel) and,
   if you like, a name of your own. Without a name the tile uses the sensor's name in Home Assistant.
4. In **Layout**, place the **Camper tanks** tile from the library's Plugins group and pick its tanks in the inspector
   (**Tanks**, in the order you want them). Nothing picked shows tank 1, 2, 3 and 4, as many as the tile has cells.

## Options

- **Tanks**: which tanks the tile shows, in this order, one per cell.
- **Show the content**: litres or kilograms under the percentage, when the tank has a content sensor and the cell has
  room.
- **Warn under** (15 %): fresh water, gas and diesel turn red at this level or under.
- **Warn over** (85 %): grey and black water turn red at this level or over.

## Good to know

- The screen reads the sensors itself through Home Assistant's API; another sensor means building the screen again.
- A sensor that is unavailable shows `--` and an empty tank; a tank without a chosen sensor says so on the tile.
- The kinds can also be changed on the screen: Settings, Plugins, Camper tanks.
- No internet service: only the Home Assistant entities you choose.
- Works on every board, the CYD included.

## A hobby project

This plugin is made and maintained in my spare time, for free and without any warranty. Bug reports and ideas are
welcome, but please keep in mind:

- There is no support and no guaranteed response time. I answer when I find the time, which can take days or weeks.
- Not every request gets built. I add what fits the plugin and what I can keep working over time.
- Issues are for bugs and concrete ideas, not for chatting. Please use the forms, keep to one topic per issue, and
  add a 👍 reaction instead of a "+1" or "same here" comment.
- Pull requests are welcome; for a larger change, open an issue first so we can agree on the way.
