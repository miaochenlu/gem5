/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * Copyright (c) 2009 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/testers/rubytest/Check.hh"

#include "base/random.hh"
#include "base/trace.hh"
#include "debug/RubyTest.hh"
#include "mem/ruby/common/SubBlock.hh"

namespace gem5
{

typedef RubyTester::SenderState SenderState;

Check::Check(Addr address, Addr pc, int _num_writers, int _num_readers,
             RubyTester* _tester)
    : m_num_writers(_num_writers), m_num_readers(_num_readers),
      m_tester_ptr(_tester)
{
    m_status = ruby::TesterStatus_Idle;

    pickValue();
    pickInitiatingNode();
    changeAddress(address);
    m_pc = pc;
    m_access_mode = ruby::RubyAccessMode(
        random_mt.random(0, ruby::RubyAccessMode_NUM - 1));
    m_store_count = 0;
}

void
Check::initiate()
{
    DPRINTF(RubyTest, "initiating\n");
    debugPrint();

    if (m_tester_ptr->getCheckFlush() && (random_mt.random(0, 0xff) == 0)) {
        initiateFlush(); // issue a Flush request from random processor
    }

    // if (m_status == ruby::TesterStatus_Idle) {
    //     initiateAction();
    // } else if (m_status == ruby::TesterStatus_Ready) {
    //     initiateCheck();
    // } else {
    //     // Pending - do nothing
    //     DPRINTF(RubyTest,
    //             "initiating action/check - failed: action/check is pending\n");
    // }

    if(m_status == ruby::TesterStatus_Idle) {
        initiateRemoteLoad1();
    } else if(m_status == ruby::TesterStatus_RemoteLoad1_Ready) {
        initiateRemoteLoad2();
    } else if(m_status == ruby::TesterStatus_RemoteLoad2_Ready) {
        initiateRequestorLoad();
    } else if(m_status == ruby::TesterStatus_Requestor_Ready) {
        initiateFlush();
    }
}

void
Check::initiateFlush()
{

    DPRINTF(RubyTest, "initiating Flush\n");

    int index = 0;
    RequestPort* port = m_tester_ptr->getWritableCpuPort(index);

    Request::Flags flags;

    RequestPtr req = std::make_shared<Request>(
            m_address, CHECK_SIZE, flags, m_tester_ptr->requestorId());
    req->setPC(m_pc);

    Packet::Command cmd;

    cmd = MemCmd::FlushReq;

    PacketPtr pkt = new Packet(req, cmd);

    // push the subblock onto the sender state.  The sequencer will
    // update the subblock on the return
    pkt->senderState = new SenderState(m_address, req->getSize());

    if (port->sendTimingReq(pkt)) {
        DPRINTF(RubyTest, "initiating flush - successful\n");
        DPRINTF(RubyTest, "status before ready: %s\n",
                ruby::TesterStatus_to_string(m_status).c_str());
        m_status = ruby::TesterStatus_Flush_Pending;
        DPRINTF(RubyTest, "Check %#x, State=Flush_Pending\n", m_address);
    } else {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt->senderState;
        delete pkt;

        DPRINTF(RubyTest, "failed to initiate check - cpu port not ready\n");
    }

    int sum = 0;
    for(int i = 0; i < 1000; i++) {
        for(int j = i * 2 + 9 / 2; j < 100000; j++) {
            for(int z = i / j * 2 + 1; z < 1000; z++) {
                sum = (i * 7 / 9 * 13 / 4 + j * z) % 1008666;
            }
        }
    }
    DPRINTF(RubyTest, "sum %d\n", sum);
    DPRINTF(RubyTest, "status after flush: %s\n",
            ruby::TesterStatus_to_string(m_status).c_str());

    debugPrint();
    // successful check complete, increment complete
    m_tester_ptr->incrementCheckCompletions();

    m_status = ruby::TesterStatus_Idle;
    DPRINTF(RubyTest, "Check %#x, State=Idle\n", m_address);
    pickValue();
}

void
Check::initiateRemoteLoad1()
{
    DPRINTF(RubyTest, "Initiating RemoteLoad1\n");
    assert(m_status == ruby::TesterStatus_Idle);

    int index = 1;
    RequestPort* port = m_tester_ptr->getReadableCpuPort(index);

    Request::Flags flags;

    // Checks are sized depending on the number of bytes written
    RequestPtr req = std::make_shared<Request>(
            m_address, CHECK_SIZE, flags, m_tester_ptr->requestorId());
    req->setPC(m_pc);

    req->setContext(index);
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    uint8_t *dataArray = new uint8_t[CHECK_SIZE];
    pkt->dataDynamic(dataArray);

    DPRINTF(RubyTest, "Seq read: index %d\n", index);

    // push the subblock onto the sender state.  The sequencer will
    // update the subblock on the return
    pkt->senderState = new SenderState(m_address, req->getSize());

    if (port->sendTimingReq(pkt)) {
        DPRINTF(RubyTest, "initiating remote load 1 - successful\n");
        DPRINTF(RubyTest, "status before remote load 2: %s\n",
                ruby::TesterStatus_to_string(m_status).c_str());
        m_status = ruby::TesterStatus_RemoteLoad1_Pending;
        DPRINTF(RubyTest, "Check %#x, State=RemoteLoad1_Pending\n", m_address);
    } else {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt->senderState;
        delete pkt;

        DPRINTF(RubyTest, "failed to initiate check - cpu port not ready\n");
    }

    DPRINTF(RubyTest, "status after remote load 1: %s\n",
            ruby::TesterStatus_to_string(m_status).c_str());
}

void
Check::initiateRemoteLoad2()
{
    DPRINTF(RubyTest, "Initiating RemoteLoad2\n");
    assert(m_status == ruby::TesterStatus_RemoteLoad1_Ready);

    int index = 2;
    RequestPort* port = m_tester_ptr->getReadableCpuPort(index);

    Request::Flags flags;

    // Checks are sized depending on the number of bytes written
    RequestPtr req = std::make_shared<Request>(
            m_address, CHECK_SIZE, flags, m_tester_ptr->requestorId());
    req->setPC(m_pc);

    req->setContext(index);
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    uint8_t *dataArray = new uint8_t[CHECK_SIZE];
    pkt->dataDynamic(dataArray);

    DPRINTF(RubyTest, "Seq read: index %d\n", index);

    // push the subblock onto the sender state.  The sequencer will
    // update the subblock on the return
    pkt->senderState = new SenderState(m_address, req->getSize());

    if (port->sendTimingReq(pkt)) {
        DPRINTF(RubyTest, "initiating remote load 2 - successful\n");
        DPRINTF(RubyTest, "status before local requestor load: %s\n",
                ruby::TesterStatus_to_string(m_status).c_str());
        m_status = ruby::TesterStatus_RemoteLoad2_Pending;
        DPRINTF(RubyTest, "Check %#x, State=RemoteLoad2_Pending\n", m_address);
    } else {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt->senderState;
        delete pkt;

        DPRINTF(RubyTest, "failed to initiate check - cpu port not ready\n");
    }

    DPRINTF(RubyTest, "status after remote load 2: %s\n",
            ruby::TesterStatus_to_string(m_status).c_str());
}

void
Check::initiateRequestorLoad()
{
    DPRINTF(RubyTest, "Initiating RequestorLoad\n");
    assert(m_status == ruby::TesterStatus_RemoteLoad2_Ready);

    int index = 0;
    RequestPort* port = m_tester_ptr->getReadableCpuPort(index);

    Request::Flags flags;

    // Checks are sized depending on the number of bytes written
    RequestPtr req = std::make_shared<Request>(
            m_address, CHECK_SIZE, flags, m_tester_ptr->requestorId());
    req->setPC(m_pc);

    req->setContext(index);
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    uint8_t *dataArray = new uint8_t[CHECK_SIZE];
    pkt->dataDynamic(dataArray);

    DPRINTF(RubyTest, "Seq read: index %d\n", index);

    // push the subblock onto the sender state.  The sequencer will
    // update the subblock on the return
    pkt->senderState = new SenderState(m_address, req->getSize());

    if (port->sendTimingReq(pkt)) {
        DPRINTF(RubyTest, "initiating requestor load - successful\n");
        DPRINTF(RubyTest, "status before flush: %s\n",
                ruby::TesterStatus_to_string(m_status).c_str());
        m_status = ruby::TesterStatus_Requestor_Pending;
        DPRINTF(RubyTest, "Check %#x, State=Requestor_Pending\n", m_address);
    } else {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt->senderState;
        delete pkt;

        DPRINTF(RubyTest, "failed to initiate check - cpu port not ready\n");
    }

    DPRINTF(RubyTest, "status after requestor load: %s\n",
            ruby::TesterStatus_to_string(m_status).c_str());
}

void
Check::initiateAction()
{
    DPRINTF(RubyTest, "initiating Action\n");
    assert(m_status == ruby::TesterStatus_Idle);

    int index = 1; // fix remote load port
    RequestPort* port = m_tester_ptr->getReadableCpuPort(index);

    Request::Flags flags;

    // if (m_tester_ptr->isInstOnlyCpuPort(index) ||
    //     (m_tester_ptr->isInstDataCpuPort(index) &&
    //      (random_mt.random(0, 0x1)))) {
    //     flags.set(Request::INST_FETCH);
    // }

    // Checks are sized depending on the number of bytes written
    RequestPtr req = std::make_shared<Request>(
            m_address, CHECK_SIZE, flags, m_tester_ptr->requestorId());
    req->setPC(m_pc);
    req->setContext(index);

    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    uint8_t *dataArray = new uint8_t[CHECK_SIZE];
    pkt->dataDynamic(dataArray);

    DPRINTF(RubyTest, "Seq read: index %d\n", index);

    pkt->senderState = new SenderState(m_address, req->getSize());

    if (port->sendTimingReq(pkt)) {
        DPRINTF(RubyTest, "initiating action - successful\n");
        DPRINTF(RubyTest, "status before action update: %s\n",
                (ruby::TesterStatus_to_string(m_status)).c_str());
        m_status = ruby::TesterStatus_Action_Pending;
        DPRINTF(RubyTest, "Check %#x, State=Action_Pending\n", m_address);
    } else {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt->senderState;
        delete pkt;

        DPRINTF(RubyTest, "failed to initiate action - sequencer not ready\n");
    }

    DPRINTF(RubyTest, "status after action update: %s\n",
            (ruby::TesterStatus_to_string(m_status)).c_str());
}

void
Check::initiateCheck()
{
    DPRINTF(RubyTest, "Initiating Check\n");
    assert(m_status == ruby::TesterStatus_Ready);

    int index = 0;
    RequestPort* port = m_tester_ptr->getReadableCpuPort(index);

    Request::Flags flags;

    // If necessary, make the request an instruction fetch
    if (m_tester_ptr->isInstOnlyCpuPort(index) ||
        (m_tester_ptr->isInstDataCpuPort(index) &&
         (random_mt.random(0, 0x1)))) {
        flags.set(Request::INST_FETCH);
    }

    // Checks are sized depending on the number of bytes written
    RequestPtr req = std::make_shared<Request>(
            m_address, CHECK_SIZE, flags, m_tester_ptr->requestorId());
    req->setPC(m_pc);

    req->setContext(index);
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    uint8_t *dataArray = new uint8_t[CHECK_SIZE];
    pkt->dataDynamic(dataArray);

    DPRINTF(RubyTest, "Seq read: index %d\n", index);

    // push the subblock onto the sender state.  The sequencer will
    // update the subblock on the return
    pkt->senderState = new SenderState(m_address, req->getSize());

    if (port->sendTimingReq(pkt)) {
        DPRINTF(RubyTest, "initiating check - successful\n");
        DPRINTF(RubyTest, "status before check update: %s\n",
                ruby::TesterStatus_to_string(m_status).c_str());
        m_status = ruby::TesterStatus_Check_Pending;
        DPRINTF(RubyTest, "Check %#x, State=Check_Pending\n", m_address);
    } else {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt->senderState;
        delete pkt;

        DPRINTF(RubyTest, "failed to initiate check - cpu port not ready\n");
    }

    DPRINTF(RubyTest, "status after check update: %s\n",
            ruby::TesterStatus_to_string(m_status).c_str());
}

void
Check::performCallback(ruby::NodeID proc, ruby::SubBlock* data, Cycles curTime)
{
    Addr address = data->getAddress();

    // This isn't exactly right since we now have multi-byte checks
    //  assert(getAddress() == address);

    assert(ruby::makeLineAddress(m_address) == ruby::makeLineAddress(address));
    assert(data != NULL);

    DPRINTF(RubyTest, "RubyTester Callback\n");
    debugPrint();

    if (m_status == ruby::TesterStatus_RemoteLoad1_Pending) {
        m_status = ruby::TesterStatus_RemoteLoad1_Ready;
    } else if (m_status == ruby::TesterStatus_RemoteLoad2_Pending) {
        m_status = ruby::TesterStatus_RemoteLoad2_Ready;
    } else if (m_status == ruby::TesterStatus_Requestor_Pending) {
        m_status = ruby::TesterStatus_Requestor_Ready;
    } 
    // else if(m_status == ruby::TesterStatus_Flush_Pending) {
    //     m_status = ruby::TesterStatus_Idle;
    //     DPRINTF(RubyTest, "Check callback\n");

    //     DPRINTF(RubyTest, "Action/check success\n");
    //     debugPrint();

    //     // successful check complete, increment complete
    //     m_tester_ptr->incrementCheckCompletions();

    //     m_status = ruby::TesterStatus_Idle;
    //     DPRINTF(RubyTest, "Check %#x, State=Idle\n", m_address);
    //     pickValue();

    // } 
    else {
        panic("Unexpected TesterStatus: %s proc: %d data: %s m_status: %s "
              "time: %d\n", *this, proc, data, m_status, curTime);
    }

    DPRINTF(RubyTest, "proc: %d, Address: 0x%x\n", proc,
            ruby::makeLineAddress(m_address));
    DPRINTF(RubyTest, "Callback done\n");
    debugPrint();
}

void
Check::changeAddress(Addr address)
{
    assert(m_status == ruby::TesterStatus_Idle ||
        m_status == ruby::TesterStatus_Ready);
    m_status = ruby::TesterStatus_Idle;
    m_address = address;
    DPRINTF(RubyTest, "Check %#x, State=Idle\n", m_address);
    m_store_count = 0;
}

void
Check::pickValue()
{
    assert(m_status == ruby::TesterStatus_Idle);
    m_value = random_mt.random(0, 0xff); // One byte
    m_store_count = 0;
}

void
Check::pickInitiatingNode()
{
    assert(m_status == ruby::TesterStatus_Idle ||
        m_status == ruby::TesterStatus_Ready);
    m_status = ruby::TesterStatus_Idle;
    m_initiatingNode = (random_mt.random(0, m_num_writers - 1));
    DPRINTF(RubyTest, "Check %#x, State=Idle, picked initiating node %d\n",
            m_address, m_initiatingNode);
    m_store_count = 0;
}

void
Check::print(std::ostream& out) const
{
    out << "["
        << m_address << ", value: "
        << (int)m_value << ", status: "
        << m_status << ", initiating node: "
        << m_initiatingNode << ", store_count: "
        << m_store_count
        << "]" << std::flush;
}

void
Check::debugPrint()
{
    DPRINTF(RubyTest,
        "[%#x, value: %d, status: %s, initiating node: %d, store_count: %d]\n",
        m_address, (int)m_value,
        ruby::TesterStatus_to_string(m_status).c_str(),
        m_initiatingNode, m_store_count);
}

} // namespace gem5
