/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2014-2016, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

/** @file
 * Implementation of unit tests for Connections
 */

#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <nupic/algorithms/Connections.hpp>

using namespace std;
using namespace nupic;
using namespace nupic::algorithms::connections;

#define EPSILON 0.0000001

namespace {

void setupSampleConnections(Connections &connections) {
  // Cell with 1 segment.
  // Segment with:
  // - 1 connected synapse: active
  // - 2 matching synapses
  const Segment segment1_1 = connections.createSegment(10);
  connections.createSynapse(segment1_1, 150, 0.85);
  connections.createSynapse(segment1_1, 151, 0.15);

  // Cell with 2 segments.
  // Segment with:
  // - 2 connected synapses: 2 active
  // - 3 matching synapses: 3 active
  const Segment segment2_1 = connections.createSegment(20);
  connections.createSynapse(segment2_1, 80, 0.85);
  connections.createSynapse(segment2_1, 81, 0.85);
  Synapse synapse = connections.createSynapse(segment2_1, 82, 0.85);
  connections.updateSynapsePermanence(synapse, 0.15);

  // Segment with:
  // - 2 connected synapses: 1 active, 1 inactive
  // - 3 matching synapses: 2 active, 1 inactive
  // - 1 non-matching synapse: 1 active
  const Segment segment2_2 = connections.createSegment(20);
  connections.createSynapse(segment2_2, 50, 0.85);
  connections.createSynapse(segment2_2, 51, 0.85);
  connections.createSynapse(segment2_2, 52, 0.15);
  connections.createSynapse(segment2_2, 53, 0.05);

  // Cell with one segment.
  // Segment with:
  // - 1 non-matching synapse: 1 active
  const Segment segment3_1 = connections.createSegment(30);
  connections.createSynapse(segment3_1, 53, 0.05);
}

void computeSampleActivity(Connections &connections) {
  vector<UInt32> input = {50, 52, 53, 80, 81, 82, 150, 151};

  vector<UInt32> numActiveConnectedSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  vector<UInt32> numActivePotentialSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  connections.computeActivity(numActiveConnectedSynapsesForSegment,
                              numActivePotentialSynapsesForSegment, input, 0.5);
}

/**
 * Creates a segment, and makes sure that it got created on the correct cell.
 */
TEST(ConnectionsTest, testCreateSegment) {
  Connections connections(1024);
  UInt32 cell = 10;

  Segment segment1 = connections.createSegment(cell);
  ASSERT_EQ(cell, connections.cellForSegment(segment1));

  Segment segment2 = connections.createSegment(cell);
  ASSERT_EQ(cell, connections.cellForSegment(segment2));

  vector<Segment> segments = connections.segmentsForCell(cell);
  ASSERT_EQ(segments.size(), 2);

  ASSERT_EQ(segment1, segments[0]);
  ASSERT_EQ(segment2, segments[1]);
}

/**
 * Creates a synapse, and makes sure that it got created on the correct
 * segment, and that its data was correctly stored.
 */
TEST(ConnectionsTest, testCreateSynapse) {
  Connections connections(1024);
  UInt32 cell = 10;
  Segment segment = connections.createSegment(cell);

  Synapse synapse1 = connections.createSynapse(segment, 50, 0.34);
  ASSERT_EQ(segment, connections.segmentForSynapse(synapse1));

  Synapse synapse2 = connections.createSynapse(segment, 150, 0.48);
  ASSERT_EQ(segment, connections.segmentForSynapse(synapse2));

  vector<Synapse> synapses = connections.synapsesForSegment(segment);
  ASSERT_EQ(synapses.size(), 2);

  ASSERT_EQ(synapse1, synapses[0]);
  ASSERT_EQ(synapse2, synapses[1]);

  SynapseData synapseData1 = connections.dataForSynapse(synapses[0]);
  ASSERT_EQ(50, synapseData1.presynapticCell);
  ASSERT_NEAR((Permanence)0.34, synapseData1.permanence, EPSILON);

  SynapseData synapseData2 = connections.dataForSynapse(synapses[1]);
  ASSERT_EQ(synapseData2.presynapticCell, 150);
  ASSERT_NEAR((Permanence)0.48, synapseData2.permanence, EPSILON);
}

/**
 * Creates a segment, destroys it, and makes sure it got destroyed along with
 * all of its synapses.
 */
TEST(ConnectionsTest, testDestroySegment) {
  Connections connections(1024);

  /*      segment1*/ connections.createSegment(10);
  Segment segment2 = connections.createSegment(20);
  /*      segment3*/ connections.createSegment(20);
  /*      segment4*/ connections.createSegment(30);

  connections.createSynapse(segment2, 80, 0.85);
  connections.createSynapse(segment2, 81, 0.85);
  connections.createSynapse(segment2, 82, 0.15);

  ASSERT_EQ(4, connections.numSegments());
  ASSERT_EQ(3, connections.numSynapses());

  connections.destroySegment(segment2);

  ASSERT_EQ(3, connections.numSegments());
  ASSERT_EQ(0, connections.numSynapses());

  vector<UInt32> numActiveConnectedSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  vector<UInt32> numActivePotentialSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  connections.computeActivity(numActiveConnectedSynapsesForSegment,
                              numActivePotentialSynapsesForSegment,
                              {80, 81, 82}, 0.5);

  ASSERT_EQ(0, numActiveConnectedSynapsesForSegment[segment2]);
  ASSERT_EQ(0, numActivePotentialSynapsesForSegment[segment2]);
}

/**
 * Creates a segment, creates a number of synapses on it, destroys a synapse,
 * and makes sure it got destroyed.
 */
TEST(ConnectionsTest, testDestroySynapse) {
  Connections connections(1024);

  Segment segment = connections.createSegment(20);
  /*      synapse1*/ connections.createSynapse(segment, 80, 0.85);
  Synapse synapse2 = connections.createSynapse(segment, 81, 0.85);
  /*      synapse3*/ connections.createSynapse(segment, 82, 0.15);

  ASSERT_EQ(3, connections.numSynapses());

  connections.destroySynapse(synapse2);

  ASSERT_EQ(2, connections.numSynapses());
  ASSERT_EQ(2, connections.synapsesForSegment(segment).size());

  vector<UInt32> numActiveConnectedSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  vector<UInt32> numActivePotentialSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  connections.computeActivity(numActiveConnectedSynapsesForSegment,
                              numActivePotentialSynapsesForSegment,
                              {80, 81, 82}, 0.5);

  ASSERT_EQ(1, numActiveConnectedSynapsesForSegment[segment]);
  ASSERT_EQ(2, numActivePotentialSynapsesForSegment[segment]);
}

/**
 * Creates segments and synapses, then destroys segments and synapses on
 * either side of them and verifies that existing Segment and Synapse
 * instances still point to the same segment / synapse as before.
 */
TEST(ConnectionsTest, PathsNotInvalidatedByOtherDestroys) {
  Connections connections(1024);

  Segment segment1 = connections.createSegment(11);
  /*      segment2*/ connections.createSegment(12);

  Segment segment3 = connections.createSegment(13);
  Synapse synapse1 = connections.createSynapse(segment3, 201, 0.85);
  /*      synapse2*/ connections.createSynapse(segment3, 202, 0.85);
  Synapse synapse3 = connections.createSynapse(segment3, 203, 0.85);
  /*      synapse4*/ connections.createSynapse(segment3, 204, 0.85);
  Synapse synapse5 = connections.createSynapse(segment3, 205, 0.85);

  /*      segment4*/ connections.createSegment(14);
  Segment segment5 = connections.createSegment(15);

  ASSERT_EQ(203, connections.dataForSynapse(synapse3).presynapticCell);
  connections.destroySynapse(synapse1);
  EXPECT_EQ(203, connections.dataForSynapse(synapse3).presynapticCell);
  connections.destroySynapse(synapse5);
  EXPECT_EQ(203, connections.dataForSynapse(synapse3).presynapticCell);

  connections.destroySegment(segment1);
  EXPECT_EQ(3, connections.synapsesForSegment(segment3).size());
  connections.destroySegment(segment5);
  EXPECT_EQ(3, connections.synapsesForSegment(segment3).size());
  EXPECT_EQ(203, connections.dataForSynapse(synapse3).presynapticCell);
}

/**
 * Destroy a segment that has a destroyed synapse and a non-destroyed synapse.
 * Make sure nothing gets double-destroyed.
 */
TEST(ConnectionsTest, DestroySegmentWithDestroyedSynapses) {
  Connections connections(1024);

  Segment segment1 = connections.createSegment(11);
  Segment segment2 = connections.createSegment(12);

  /*      synapse1_1*/ connections.createSynapse(segment1, 101, 0.85);
  Synapse synapse2_1 = connections.createSynapse(segment2, 201, 0.85);
  /*      synapse2_2*/ connections.createSynapse(segment2, 202, 0.85);

  ASSERT_EQ(3, connections.numSynapses());

  connections.destroySynapse(synapse2_1);

  ASSERT_EQ(2, connections.numSegments());
  ASSERT_EQ(2, connections.numSynapses());

  connections.destroySegment(segment2);

  EXPECT_EQ(1, connections.numSegments());
  EXPECT_EQ(1, connections.numSynapses());
}

/**
 * Destroy a segment that has a destroyed synapse and a non-destroyed synapse.
 * Create a new segment in the same place. Make sure its synapse count is
 * correct.
 */
TEST(ConnectionsTest, ReuseSegmentWithDestroyedSynapses) {
  Connections connections(1024);

  Segment segment = connections.createSegment(11);

  Synapse synapse1 = connections.createSynapse(segment, 201, 0.85);
  /*      synapse2*/ connections.createSynapse(segment, 202, 0.85);

  connections.destroySynapse(synapse1);

  ASSERT_EQ(1, connections.numSynapses(segment));

  connections.destroySegment(segment);
  Segment reincarnated = connections.createSegment(11);

  EXPECT_EQ(0, connections.numSynapses(reincarnated));
  EXPECT_EQ(0, connections.synapsesForSegment(reincarnated).size());
}

/**
 * Creates a synapse and updates its permanence, and makes sure that its
 * data was correctly updated.
 */
TEST(ConnectionsTest, testUpdateSynapsePermanence) {
  Connections connections(1024);
  Segment segment = connections.createSegment(10);
  Synapse synapse = connections.createSynapse(segment, 50, 0.34);

  connections.updateSynapsePermanence(synapse, 0.21);

  SynapseData synapseData = connections.dataForSynapse(synapse);
  ASSERT_NEAR(synapseData.permanence, (Real)0.21, EPSILON);

  // Test permanence floor
  connections.updateSynapsePermanence(synapse, -0.02f);
  synapseData = connections.dataForSynapse(synapse);
  ASSERT_EQ(synapseData.permanence, (Real)0.0f );

  connections.updateSynapsePermanence(synapse, -EPSILON / 10.);
  synapseData = connections.dataForSynapse(synapse);
  ASSERT_EQ(synapseData.permanence, (Real)0.0f );

  // Test permanence ceiling
  connections.updateSynapsePermanence(synapse, 1.02f);
  synapseData = connections.dataForSynapse(synapse);
  ASSERT_EQ(synapseData.permanence, (Real)1.0f );

  connections.updateSynapsePermanence(synapse, 1.0f + EPSILON / 10.);
  synapseData = connections.dataForSynapse(synapse);
  ASSERT_EQ(synapseData.permanence, (Real)1.0f );
}

/**
 * Creates a sample set of connections, and makes sure that computing the
 * activity for a collection of cells with no activity returns the right
 * activity data.
 */
TEST(ConnectionsTest, testComputeActivity) {
  Connections connections(1024);

  // Cell with 1 segment.
  // Segment with:
  // - 1 connected synapse: active
  // - 2 matching synapses: active
  const Segment segment1_1 = connections.createSegment(10);
  connections.createSynapse(segment1_1, 150, 0.85);
  connections.createSynapse(segment1_1, 151, 0.15);

  // Cell with 1 segments.
  // Segment with:
  // - 2 connected synapses: 2 active
  // - 3 matching synapses: 3 active
  const Segment segment2_1 = connections.createSegment(20);
  connections.createSynapse(segment2_1, 80, 0.85);
  connections.createSynapse(segment2_1, 81, 0.85);
  Synapse synapse = connections.createSynapse(segment2_1, 82, 0.85);
  connections.updateSynapsePermanence(synapse, 0.15);

  vector<UInt32> input = {50, 52, 53, 80, 81, 82, 150, 151};

  vector<UInt32> numActiveConnectedSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  vector<UInt32> numActivePotentialSynapsesForSegment(
      connections.segmentFlatListLength(), 0);
  connections.computeActivity(numActiveConnectedSynapsesForSegment,
                              numActivePotentialSynapsesForSegment, input, 0.5);

  ASSERT_EQ(1, numActiveConnectedSynapsesForSegment[segment1_1]);
  ASSERT_EQ(2, numActivePotentialSynapsesForSegment[segment1_1]);

  ASSERT_EQ(2, numActiveConnectedSynapsesForSegment[segment2_1]);
  ASSERT_EQ(3, numActivePotentialSynapsesForSegment[segment2_1]);
}


TEST(ConnectionsTest, testAdaptSynapses) {
  UInt numColumns = 4;
  UInt numInputs = 8;
  Connections con(numColumns);

  vector<UInt> activeColumns;
  SDR input({numInputs});

  UInt potentialArr[4][8] =  {{1, 1, 1, 1, 0, 0, 0, 0},
                              {1, 0, 0, 0, 1, 1, 0, 1},
                              {0, 0, 1, 0, 0, 0, 1, 0},
                              {1, 0, 0, 0, 0, 0, 1, 0}};

  Real permanences[4][8] = {
      {0.200, 0.120, 0.090, 0.060, 0.000, 0.000, 0.000, 0.000},
      {0.150, 0.000, 0.000, 0.000, 0.180, 0.120, 0.000, 0.450},
      {0.000, 0.000, 0.004, 0.000, 0.000, 0.000, 0.910, 0.000},
      {0.070, 0.000, 0.000, 0.000, 0.000, 0.000, 0.178, 0.000}};

  Real truePerms[4][8] = {
      {0.300, 0.110, 0.080, 0.160, 0.000, 0.000, 0.000, 0.000},
      // Inc    Dec    Dec    Inc      -      -      -     -
      {0.250, 0.000, 0.000, 0.000, 0.280, 0.110, 0.000, 0.440},
      // Inc      -      -     -      Inc    Dec    -     Dec
      {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 1.000, 0.000},
      //   -      -   Floor     -     -     -    Ceiling   -
      {0.070, 0.000, 0.000, 0.000, 0.000, 0.000, 0.178, 0.000}};
      //   -      -      -      -      -      -      -      -

  for (UInt column = 0; column < numColumns; column++) {
    Segment seg = con.createSegment(column);
    for(UInt inp = 0; inp < numInputs; inp++) {
      if( potentialArr[column][inp] )
        con.createSynapse(seg, inp, permanences[column][inp]);
    }
  }

  input.setDense(SDR_dense_t({ 1, 0, 0, 1, 1, 0, 1, 0 }));
  activeColumns.assign({0, 1, 2});

  for(UInt column : activeColumns)
    con.adaptSegment(column, input, .1, .01);

  for (UInt column = 0; column < numColumns; column++) {
    vector<Real> perms( numInputs, 0.0f );
    for( Synapse syn : con.synapsesForSegment(column) ) {
      auto synData = con.dataForSynapse( syn );
      perms[ synData.presynapticCell ] = synData.permanence;
    }
    for(UInt i = 0; i < numInputs; i++)
      ASSERT_NEAR( truePerms[column][i], perms[i], EPSILON );
  }
}


TEST(ConnectionsTest, testRaisePermanencesToThreshold) {
  FAIL(); /*
  SpatialPooler sp;
  UInt stimulusThreshold = 3;
  Real synPermConnected = 0.1;
  Real synPermBelowStimulusInc = 0.01;
  UInt numInputs = 5;
  UInt numColumns = 7;
  setup(sp, numInputs, numColumns);
  sp.setStimulusThreshold(stimulusThreshold);
  sp.setSynPermConnected(synPermConnected);
  sp.setSynPermBelowStimulusInc(synPermBelowStimulusInc);

  UInt potentialArr[7][5] = {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1},
                             {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 0, 0, 1},
                             {0, 1, 1, 1, 0}};

  Real permArr[7][5] = {{0.0, 0.11, 0.095, 0.092, 0.01},
                        {0.12, 0.15, 0.02, 0.12, 0.09},
                        {0.51, 0.081, 0.025, 0.089, 0.31},
                        {0.18, 0.0601, 0.11, 0.011, 0.03},
                        {0.011, 0.011, 0.011, 0.011, 0.011},
                        {0.12, 0.056, 0, 0, 0.078},
                        {0, 0.061, 0.07, 0.14, 0}};

  Real truePerm[7][5] = {
      {0.01, 0.12, 0.105, 0.102, 0.02},    // incremented once
      {0.12, 0.15, 0.02, 0.12, 0.09},      // no change
      {0.53, 0.101, 0.045, 0.109, 0.33},   // increment twice
      {0.22, 0.1001, 0.15, 0.051, 0.07},   // increment four times
      {0.101, 0.101, 0.101, 0.101, 0.101}, // increment 9 times
      {0.17, 0.106, 0, 0, 0.128},          // increment 5 times
      {0, 0.101, 0.11, 0.18, 0}};          // increment 4 times

  UInt trueConnectedCount[7] = {3, 3, 4, 3, 5, 3, 3};

  for (UInt i = 0; i < numColumns; i++) {
    vector<Real> perm;
    vector<UInt> potential;
    perm.assign(&permArr[i][0], &permArr[i][numInputs]);
    for (UInt j = 0; j < numInputs; j++) {
      if (potentialArr[i][j] > 0) {
        potential.push_back(j);
      }
    }
    UInt connected = sp.raisePermanencesToThreshold_(perm, potential);
    ASSERT_TRUE(check_vector_eq(truePerm[i], perm));
    ASSERT_TRUE(connected == trueConnectedCount[i]);
  }
  */
}


TEST(ConnectionsTest, testBumpUpWeakColumns) {
  FAIL(); /*
  SpatialPooler sp;
  UInt numInputs = 8;
  UInt numColumns = 5;
  setup(sp, numInputs, numColumns);
  sp.setSynPermBelowStimulusInc(0.01);
  sp.setSynPermTrimThreshold(0.05);
  Real overlapDutyCyclesArr[] = {0, 0.009, 0.1, 0.001, 0.002};
  sp.setOverlapDutyCycles(overlapDutyCyclesArr);
  Real minOverlapDutyCyclesArr[] = {0.01, 0.01, 0.01, 0.01, 0.01};
  sp.setMinOverlapDutyCycles(minOverlapDutyCyclesArr);

  UInt potentialArr[5][8] = {{1, 1, 1, 1, 0, 0, 0, 0},
                             {1, 0, 0, 0, 1, 1, 0, 1},
                             {0, 0, 1, 0, 1, 1, 1, 0},
                             {1, 1, 1, 0, 0, 0, 1, 0},
                             {1, 1, 1, 1, 1, 1, 1, 1}};

  Real permArr[5][8] = {
      {0.200, 0.120, 0.090, 0.040, 0.000, 0.000, 0.000, 0.000},
      {0.150, 0.000, 0.000, 0.000, 0.180, 0.120, 0.000, 0.450},
      {0.000, 0.000, 0.074, 0.000, 0.062, 0.054, 0.110, 0.000},
      {0.051, 0.000, 0.000, 0.000, 0.000, 0.000, 0.178, 0.000},
      {0.100, 0.738, 0.085, 0.002, 0.052, 0.008, 0.208, 0.034}};

  Real truePermArr[5][8] = {
      {0.210, 0.130, 0.100, 0.000, 0.000, 0.000, 0.000, 0.000},
      //  Inc    Inc    Inc    Trim    -     -     -    -
      {0.160, 0.000, 0.000, 0.000, 0.190, 0.130, 0.000, 0.460},
      //  Inc   -     -    -     Inc   Inc    -     Inc
      {0.000, 0.000, 0.074, 0.000, 0.062, 0.054, 0.110, 0.000}, // unchanged
      //  -    -     -    -     -    -     -    -
      {0.061, 0.000, 0.000, 0.000, 0.000, 0.000, 0.188, 0.000},
      //   Inc   Trim    Trim    -     -      -     Inc     -
      {0.110, 0.748, 0.095, 0.000, 0.062, 0.000, 0.218, 0.000}};

  for (UInt i = 0; i < numColumns; i++) {
    sp.setPotential(i, potentialArr[i]);
    sp.setPermanence(i, permArr[i]);
    Real perm[8];
    sp.getPermanence(i, perm);
  }

  sp.bumpUpWeakColumns_();

  for (UInt i = 0; i < numColumns; i++) {
    Real perm[8];
    sp.getPermanence(i, perm);
    ASSERT_TRUE(check_vector_eq(truePermArr[i], perm, numInputs));
  }
  */
}


TEST(ConnectionsTest, testConnectedCount) {
  FAIL();
}


/**
 * Test the mapSegmentsToCells method.
 */
TEST(ConnectionsTest, testMapSegmentsToCells) {
  Connections connections(1024);

  const Segment segment1 = connections.createSegment(42);
  const Segment segment2 = connections.createSegment(42);
  const Segment segment3 = connections.createSegment(43);

  const vector<Segment> segments = {segment1, segment2, segment3, segment1};
  vector<CellIdx> cells(segments.size());

  connections.mapSegmentsToCells(
      segments.data(), segments.data() + segments.size(), cells.data());

  const vector<CellIdx> expected = {42, 42, 43, 42};
  ASSERT_EQ(expected, cells);
}

bool TEST_EVENT_HANDLER_DESTRUCTED = false;

class TestConnectionsEventHandler : public ConnectionsEventHandler {
public:
  TestConnectionsEventHandler()
      : didCreateSegment(false), didDestroySegment(false),
        didCreateSynapse(false), didDestroySynapse(false),
        didUpdateSynapsePermanence(false) {}

  virtual ~TestConnectionsEventHandler() {
    TEST_EVENT_HANDLER_DESTRUCTED = true;
  }

  virtual void onCreateSegment(Segment segment) { didCreateSegment = true; }

  virtual void onDestroySegment(Segment segment) { didDestroySegment = true; }

  virtual void onCreateSynapse(Synapse synapse) { didCreateSynapse = true; }

  virtual void onDestroySynapse(Synapse synapse) { didDestroySynapse = true; }

  virtual void onUpdateSynapsePermanence(Synapse synapse,
                                         Permanence permanence) {
    didUpdateSynapsePermanence = true;
  }

  bool didCreateSegment;
  bool didDestroySegment;
  bool didCreateSynapse;
  bool didDestroySynapse;
  bool didUpdateSynapsePermanence;
};

/**
 * Make sure each event handler gets called.
 */
TEST(ConnectionsTest, subscribe) {
  Connections connections(1024);

  TestConnectionsEventHandler *handler = new TestConnectionsEventHandler();
  auto token = connections.subscribe(handler);

  ASSERT_FALSE(handler->didCreateSegment);
  Segment segment = connections.createSegment(42);
  EXPECT_TRUE(handler->didCreateSegment);

  ASSERT_FALSE(handler->didCreateSynapse);
  Synapse synapse = connections.createSynapse(segment, 41, 0.50);
  EXPECT_TRUE(handler->didCreateSynapse);

  ASSERT_FALSE(handler->didUpdateSynapsePermanence);
  connections.updateSynapsePermanence(synapse, 0.60);
  EXPECT_TRUE(handler->didUpdateSynapsePermanence);

  ASSERT_FALSE(handler->didDestroySynapse);
  connections.destroySynapse(synapse);
  EXPECT_TRUE(handler->didDestroySynapse);

  ASSERT_FALSE(handler->didDestroySegment);
  connections.destroySegment(segment);
  EXPECT_TRUE(handler->didDestroySegment);

  connections.unsubscribe(token);
}

/**
 * Make sure the event handler is destructed on unsubscribe.
 */
TEST(ConnectionsTest, unsubscribe) {
  Connections connections(1024);
  TestConnectionsEventHandler *handler = new TestConnectionsEventHandler();
  auto token = connections.subscribe(handler);

  TEST_EVENT_HANDLER_DESTRUCTED = false;
  connections.unsubscribe(token);
  EXPECT_TRUE(TEST_EVENT_HANDLER_DESTRUCTED);
}

/**
 * Creates a sample set of connections, and makes sure that we can get the
 * correct number of segments.
 */
TEST(ConnectionsTest, testNumSegments) {
  Connections connections(1024);
  setupSampleConnections(connections);

  ASSERT_EQ(4, connections.numSegments());
}

/**
 * Creates a sample set of connections, and makes sure that we can get the
 * correct number of synapses.
 */
TEST(ConnectionsTest, testNumSynapses) {
  Connections connections(1024);
  setupSampleConnections(connections);

  ASSERT_EQ(10, connections.numSynapses());
}

/**
 * Creates a sample set of connections with destroyed segments/synapses,
 * computes sample activity, and makes sure that we can save to a
 * filestream and load it back correctly.
 */
TEST(ConnectionsTest, testSaveLoad) {
  Connections c1(1024), c2;
  setupSampleConnections(c1);

  auto segment = c1.createSegment(10);

  c1.createSynapse(segment, 400, 0.5);
  c1.destroySegment(segment);

  computeSampleActivity(c1);

  {
    stringstream ss;
    c1.save(ss);
    c2.load(ss);
  }

  ASSERT_EQ(c1, c2);
}

} // namespace
