#include "Checkpoint_rec.h"

namespace libbyzea {

void Checkpoint_rec::print() {
  printf("Checkpoint record: %d blocks \n", parts.size());
  MapGenerator<PartKey, Part*> g(parts);
  PartKey k;
  Part* p;
  while (g.get(k, p)) {
    printf("Block: level= %d index=  %d  ", k.level, k.index);
    printf("last mod=%qd ", p->lm);
    p->d.print();
    printf("\n");
  }
}

void Checkpoint_rec::clear() {
  if (!is_cleared()) {
    MapGenerator<PartKey, Part*> g(parts);
    PartKey k;
    Part* p;
    while (g.get(k, p)) {
      if (k.level == PLevels - 1) {
        //	/* debug */ fprintf(stderr, "Clearing leaf %d\t", k.index);
        delete ((BlockCopy*)p);
      } else
        delete p;
      g.remove();
    }
    sd.zero();
  }
}

}  // namespace libbyzea
