Armor Lab https://downfall318.github.io/ArPen/

Profile initialization creates Materials.json, ArmorProfiles.json and AmmoProfiles.json only when each file is absent. Existing files are loaded without supplementation, migration or rewriting. All profile schemas and the item persistence layout are unversioned.

The fixed item persistence layout does not read older ArPen saves. Existing world item persistence written by a previous layout must be reset or restored using the matching old mod build; deleting profile JSON files does not convert saved items.
