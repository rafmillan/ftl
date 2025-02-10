# FTL
## Overall Architecture and Structure
The NAND structure and mapping tables (primary and overflow) are allocated statically. This is acceptable for simulation or embedded targets with fixed resource limits. In a more dynamic or scalable design, you might consider dynamic allocation or configuration-based sizing.

## Basic Commands
`write_lba(lba)`   
`read_lba(lba)`  
`manual_markbad(block)`  
`erase_block(block)`  


## NAND Policies
### Wear Leveling
**DynamicWear Leveling**: When choosing a free block for writing, the block with the lowest erase count is selected first.


### [WIP] Bad Blocks
**P/E Cycles**: If a block exceeds the MAX_ERASE_CYCLES, it will be marked bad and corresponding lba is invalidated.  

**Manual Mark Bad**: For testing purposes, manually marks a block as bad. Data is still considered valid and migrated to a new physical location, LBA table is updated

## Testing
#### Test Cases
- [ ] nand write page  
- [ ] nand read page  
- [ ] nand erase block  

logical to physical mapping  
- [ ] insert LBA
- [ ] delete LBA
- [ ] trim LBA

other
- [x] nand basic
- [x] lba basic
- [x] lba trim
- [x] wear level basic
- [x] mark bad basic 
