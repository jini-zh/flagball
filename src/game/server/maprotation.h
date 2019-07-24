#ifndef GAME_SERVER_MAPROTATION_H
#define GAME_SERVER_MAPROTATION_H

class IMapRotation {
 public:
  virtual const char* NextMap(const char* rotation, const char* current) = 0;
  virtual ~IMapRotation() {};
};

IMapRotation *RandomMapRotation();

#endif
