#include "congestion_control.h"
#include "globals.h"
#include <iostream>

CongestionControl::CongestionControl(
            CCMode mode /*=SS*/,
            uint16_t cwnd /*=PACKET_DATA_SIZE*/)
    : m_mode(mode), m_cwnd(cwnd)
{}

void CongestionControl::setMode(CCMode mode) {
    m_mode = mode;
}

bool CongestionControl::increaseCwnd() {
    switch (CCmode) {
        case SS: {
            if (m_cwnd < ssthresh) {
                // increase by PACKET_DATA_SIZE
                m_cwnd += PACKET_DATA_SIZE;    
            }
            else {
                std::cerr << "Congestion Control mode should be CA\n";
                return false;
            }
        } break;
        case CA: {
            // TODO: increase by some fraction of m_cwnd
        } break;
        default: {
            std::cerr << "Congestion Control mode in invalid\n";
            return false;
        }
    }

    return true;
}
