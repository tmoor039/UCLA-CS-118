#ifndef CONGESTION_CONTROL_H
#define CONGESTION_CONTROL_H

class CongestionControl {
public:
    CongestionControl(CCMode mode = SS, uint16_t cwnd = MIN_CWND);

    // mutators
    void setMode(CCMode mode);

    // to call when server receives ack
    bool increaseCwnd();

    // accessors
    CCMode getMode() { return m_mode; }
    uint16_t getCwnd() { return m_cwnd; }

private:
    // Slow Start (SS) or Congestion Avoidance (CA)
    CCMode m_mode;

    // unit of bytes
    uint16_t m_cwnd;
};

#endif // CONGESTION_CONTROL_H
