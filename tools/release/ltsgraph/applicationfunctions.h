#ifndef APPLICATIONFUNCTIONS_H
#define APPLICATIONFUNCTIONS_H

namespace ApplicationFunctions
{
  inline void apply_forces(QVector3D& pos, 
                    const QVector3D& f,
                    const float speed,
                    const float temperature)
  {
    const float scaling = 1.0f;

    float L = f.length();
    if (L == 0)
        return;

    pos = pos + (f * speed * scaling * temperature / L);
  }
}; // namespace ApplicationFunctions

#endif
