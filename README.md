# ftl
## 1. Overall Architecture and Structure
**Static Allocation:**
The NAND structure and mapping tables (primary and overflow) are allocated statically. This is acceptable for simulation or embedded targets with fixed resource limits. In a more dynamic or scalable design, you might consider dynamic allocation or configuration-based sizing.

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
