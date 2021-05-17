// Author: Prime541
// Ranked: 1717 / 6867
// Ligue: Gold
// Algorithm: heuristic
// Date: 2021-05-17
// https://www.codingame.com/forum/t/spring-challenge-2021-feedbacks-strategies/190849

#define NDEBUG // No Debug, eventually ignores assert() from #include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>// memset
using namespace std;

constexpr char DAYS = 24;
constexpr bool ME = 1;
constexpr bool OPP = 1 - ME;
constexpr char treeBaseCost[4] = {0, 1, 3, 7};

struct Cell {
    unsigned char index; // 0 is the center cell, the next cells spiral outwards
    unsigned char richness; // 0 if the cell is unusable, 1-3 for usable cells
    char neighs[6+12+18]; //   6: the index of the neighbouring cell for each direction
                          // +12: at distance of 2
                          // +18: at distance of 3
};
istream& operator>>(istream& in, Cell& o) {
    short s;
    in >> s; o.index = s;
    in >> s; o.richness = s;
    for (int i = 0; i < 6; ++i) {
        in >> s; o.neighs[i] = s;
    }
    return in;
}
ostream& operator<<(ostream& out, const Cell& o) {
    out << (short)o.index << ' '
        << (short)o.richness;
    for (int i = 0; i < 6; ++i) {
        out << ' ' << (short)o.neighs[i];
    }
    return out;
}

// static list of soil cells
vector<Cell> cells;
void fillNeighbours() {
    // dist 2
    for (int i = 0; i < cells.size(); ++i) {
        Cell& c = cells[i];
        for (char j = 0; j < 6; ++j) {//0-1,1-2,2-3,3-4,4-5,5-0
            c.neighs[6+j*2  ] = c.neighs[j] != -1 ? cells[c.neighs[j]].neighs[ j     ] : -1;
            c.neighs[6+j*2+1] = c.neighs[j] != -1 ? cells[c.neighs[j]].neighs[(j+1)%6] : -1;
        }
    }
    // dist 3
    for (int i = 0; i < cells.size(); ++i) {
        Cell& c = cells[i];
        for (char j = 0; j < 6; ++j) {//6+: 0-1-2,2-3-4,4-5-6,6-7-8,8-9-10,10-11-0
            c.neighs[6+12+j*3  ] = c.neighs[j] != -1 ? cells[c.neighs[j]].neighs[6+(j*2  )%12] : -1;
            c.neighs[6+12+j*3+1] = c.neighs[j] != -1 ? cells[c.neighs[j]].neighs[6+(j*2+1)%12] : -1;
            c.neighs[6+12+j*3+2] = c.neighs[j] != -1 ? cells[c.neighs[j]].neighs[6+(j*2+2)%12] : -1;
        }
    }
#ifndef NDEBUG
    for (int i = 0; i < cells.size(); ++i) {
        cerr << (short)cells[i].index << ':';
        for (int j = 0; j < 6+12+18; ++j) {
            cerr << ' ' << (short)cells[i].neighs[j];
        }
        cerr << endl;
    }
#endif
}
istream& operator>>(istream& in, vector<Cell>& o) {
    short s; // 37
    in >> s; in.ignore();
    o.resize(s);
    for (int i = 0; i < s; ++i) {
        in >> o[i]; in.ignore();
    }
    fillNeighbours();
    return in;
}
ostream& operator<<(ostream& out, const vector<Cell>& o) {
    out << o.size();
    for (int i = 0; i < o.size(); ++i) {
        out << endl << o[i];
    }
    return out;
}

struct Tree {// 2 bytes, this was useful in MC to compare trees with a *reinterpret_cast<const short*>(this)
    char cellIndex; // location of this tree (6 bits)
    unsigned char size:4; // size of this tree: 0-3 (2 bits)
    bool owner:2; // 1 if this is your tree, 0 otherwise (1 bit)
    bool isDormant:2; // 1 if this tree is dormant (1 bit)
    bool operator==(const Tree& o) const;
    bool operator<(const Tree& o) const;
};
istream& operator>>(istream& in, Tree& o) {
    short s; bool b;
    in >> s; o.cellIndex = s;
    in >> s; o.size = s;
    in >> b; o.owner = b;
    in >> b; o.isDormant = b;
    return in;
}
ostream& operator<<(ostream& out, const Tree& o) {
    out << (short)o.cellIndex << ' '
        << (short)o.size << ' '
        << o.owner << ' '
        << o.isDormant;
    return out;
}
bool Tree::operator<(const Tree& o) const {
    return *reinterpret_cast<const short*>(this) < *reinterpret_cast<const short*>(&o);
}
bool Tree::operator==(const Tree& o) const {
    return *reinterpret_cast<const short*>(this) == *reinterpret_cast<const short*>(&o);
}

struct Turn {
    char day; // the game lasts 24 days: 0-23 (5 bits)
    char nutrients; // the base score you gain from the next COMPLETE action: 0-20? (5 bites)
    unsigned char suns[2]; // your sun points; opponent's sun points
    unsigned char scores[2]; // your current score; opponent's score
    bool isWaitings[2];// whether you are asleep; whether your opponent is asleep until the next day (1 bit * 2)
    vector<Tree> trees;
    char treeCounts[2][4];// cache (6 bits * 2*4)
    char treeNDormCounts[2][4];// cache (6 bits * 2*4)
    char cellToTree[37];// cache
    void fillCaches();
};
istream& operator>>(istream& in, Turn& o) {
    short s;
    in >> s; o.day = s; cin.ignore();
    in >> s; o.nutrients = s; cin.ignore();
    in >> s; o.suns[ME] = s;
    in >> s; o.scores[ME] = s; cin.ignore();
    in >> s; o.suns[OPP] = s;
    in >> s; o.scores[OPP] = s;
    o.isWaitings[ME] = false;
    in >> o.isWaitings[OPP]; cin.ignore();
    in >> s; cin.ignore();// the current amount of trees
    o.trees.resize(s);
    for (int i = 0; i < s; ++i) {
        cin >> o.trees[i]; cin.ignore();
    }
    in >> s; cin.ignore();// all legal actions
#ifndef NDEBUG
    cerr << "Possible actions: " << s << endl;
#endif
    static string possibleAction;
    for (int i = 0; i < s; i++) {
        getline(cin, possibleAction); // try printing something from here to start with
#ifndef NDEBUG
        cerr << possibleAction << endl;
#endif
    }
    sort(o.trees.begin(), o.trees.end());
    o.fillCaches();
    return in;
}
ostream& operator<<(ostream& out, const Turn& o) {
    out << (short)o.day << endl
        << (short)o.nutrients << endl
        << (short)o.suns[ME] << ' '
        << (short)o.scores[ME] << endl
        << (short)o.suns[OPP] << ' '
        << (short)o.scores[OPP] << ' '
        << o.isWaitings[OPP] << endl
        << o.trees.size() << endl;
    for (int i = 0; i < o.trees.size(); ++i) {
        out << o.trees[i] << endl;
    }
    out << 0;
    return out;
}
void Turn::fillCaches() {
    memset(treeCounts, 0, sizeof(treeCounts));
    memset(treeNDormCounts, 0, sizeof(treeNDormCounts));
    memset(cellToTree, -1, sizeof(cellToTree));
    for (char i = 0; i < trees.size(); ++i) {
        const Tree& tree = trees[i];
        ++treeCounts[tree.owner][tree.size];
        if (!tree.isDormant) {
            ++treeNDormCounts[tree.owner][tree.size];
        }
        cellToTree[tree.cellIndex] = i;
    }
}

constexpr unsigned char WAIT = 0;
constexpr unsigned char COMPLETE = 1;
constexpr unsigned char GROW = 2;
constexpr unsigned char SEED = 3;
struct Action {// 2 bytes, this was useful in MC to compare actions with a *reinterpret_cast<const short*>(this)
    unsigned char type:2;//(2 bits)
    unsigned char idx0:6;//(6 bits)
    unsigned char idx1;//(6 bits)
    Action() {}
    Action(const string& s);
    Action(unsigned char t) : type(t) {}
    Action(unsigned char t, unsigned char i0) : type(t) {idx0 = i0;}
    Action(unsigned char t, unsigned char i0, unsigned char i1) : type(t), idx0(i0), idx1(i1) {}
    Action(const Action& o) : type(o.type), idx0(o.idx0), idx1(o.idx1) {}
    Action& operator=(const Action& o) {if (this != &o) {type=o.type; idx0=o.idx0; idx1=o.idx1;} return *this;}
};
ostream& operator<<(ostream& out, const Action& o) {
    switch (o.type) {
        case WAIT: out << "WAIT"; return out;
        case COMPLETE: out << "COMPLETE " << (short)o.idx0; return out;
        case GROW: out << "GROW " << (short)o.idx0; return out;
        case SEED: out << "SEED " << (short)o.idx0 << ' ' << (short)o.idx1; return out;
        default: cerr << "Unexpected Action.type '" << o.type << "' == " << (short)o.type << endl;
    }
    return out;
}
Action::Action(const string& s) {
    if (s.find("WAIT") == 0) {
        type = WAIT;
    } else if (s.find("COMPLETE ") == 0) {
        type = COMPLETE;
        idx0 = atoi(s.c_str() + 9);
    } else if (s.find("GROW ") == 0) {
        type = GROW;
        idx0 = atoi(s.c_str() + 5);
    } else if (s.find("SEED ") == 0) {
        type = COMPLETE;
        idx0 = atoi(s.c_str() + 5);
        idx1 = atoi(s.c_str() + s.find_first_of(' ', 5) + 1);
    } else {
        cerr << "Unexpected Action string: " << s << endl;
    }
}

double countTreeInSun(const Turn& turn, char cellIndex, char size, char turnOffset=0 /* 1, starting nextDay */, char duration=1 /* 6, full cycle */, double decay=1.0) {
    double value = 0;
    double certainty = 1.0;
    for (int day = turn.day; day < turn.day + turnOffset + duration; ++day) {
        if (day < DAYS) {
            if (day >= turn.day + turnOffset) {
                const Cell& treeCell = cells[cellIndex];
                const char sunDir = (day + 3) % 6;
                char treeIdx;
                if (!(treeCell.neighs[sunDir] > -1 && (treeIdx = turn.cellToTree[treeCell.neighs[sunDir]]) > -1 && turn.trees[treeIdx].size >= size && turn.trees[treeIdx].size >= 1) &&
                    !(treeCell.neighs[6+sunDir*2] > -1 && (treeIdx = turn.cellToTree[treeCell.neighs[6+sunDir*2]]) > -1 && turn.trees[treeIdx].size >= size && turn.trees[treeIdx].size >= 2) &&
                    !(treeCell.neighs[6+12+sunDir*3] > -1 && (treeIdx = turn.cellToTree[treeCell.neighs[6+12+sunDir*3]]) > -1 && turn.trees[treeIdx].size >= size && turn.trees[treeIdx].size >= 3)) {
                    value += certainty;
                }
            }
            certainty *= decay;
        } else {
            break;
        }
    }
#ifndef NDEBUG
    cerr << "estTreeSun(day="<<(short)turn.day<<", cell="<<(short)cellIndex<<", sz="<<(short)size<<", offset="<<(short)turnOffset<<", dur="<<(short)duration<<", decay="<<(double)decay<<") = " << value << endl;
#endif
    return value;
}

Action getNextAction(const Turn& turn, bool player/* OPP=0 or ME=1 */) {
    if (turn.isWaitings[player]) {
        return Action(WAIT);// this was useful in MC to generate and simulate next turn
    }

#ifndef NDEBUG
    cerr << "Tree counts["<<player<<"]:";
    for (int i = 0; i < 4; ++i) {
        cerr << ' ' << (short)turn.treeCounts[player][i];
    }
    cerr << "Non dormant tree counts["<<player<<"]:";
    for (int i = 0; i < 4; ++i) {
        cerr << ' ' << (short)turn.treeNDormCounts[player][i];
    }
    cerr << endl;
#endif

    double maxValue = 0;
    char maxCellIdx = 0;
    // DONE: do not COMPLETE too soon
    bool sellItAll = (turn.treeCounts[player][3] >= DAYS - turn.day || turn.day >= DAYS - 1 - 6);
    cerr << "sellItAll["<<player<<"]: " << sellItAll << " = (" << (short)turn.treeCounts[player][3] << " >= " << (short)DAYS << " - " << (short)turn.day << ')' << endl;
    if (sellItAll && turn.treeCounts[player][3] > 0) {
        // Generate the best COMPLETE action
        for (char i = 0; i < turn.trees.size(); ++i) {
            const Tree& tree = turn.trees[i];
            if (tree.owner == player && tree.size == 3 && !tree.isDormant && turn.suns[player] >= 4) {
                double value = ((double)cells[tree.cellIndex].richness - 1) * 2 + turn.nutrients - countTreeInSun(turn, tree.cellIndex, 3, 1, 6, 0.9);
                if (value > maxValue) {
                    maxValue = value;
                    maxCellIdx = tree.cellIndex;
                }
            }
        }
        if (maxValue > 0) {
            return Action(COMPLETE, maxCellIdx);
        }
    } else {
        // Generate the best GROW action
        for (char sz = 3; sz >= 1; --sz) {
            // TODO: GROW to put shadow on an OPP tree?
            if (sz == 3 && turn.day <= 4) {
                cerr << "Avoid to GROW big too soon" << endl;
                continue;
            }
            // DONE: do not GROW trees that I cannot COMPLETE
            if (3-sz < DAYS-turn.day-1) {
                if (turn.suns[player] >= treeBaseCost[sz] + turn.treeCounts[player][sz]) {
                    // NOT CONVINCING: prefer to GROW 2 trees than to GROW 1 big tree in the early game
                    /*// only 1 GROW 2->3 vs many GROW (preferred in early game)
                    if (sz == 3 && turn.day < DAYS / 2) {
                        short tmp_suns = turn.suns[player];
                        short grox_2_3_count = 0;
                        while (grox_2_3_count < (short)turn.treeNDormCounts[player][2] && tmp_suns >= (short)treeBaseCost[3] + turn.treeCounts[player][3] + grox_2_3_count) {
                            tmp_suns -= (short)treeBaseCost[3] + turn.treeCounts[player][3] + grox_2_3_count;
                            ++grox_2_3_count;
                        }
                        if (grox_2_3_count == 1 && (turn.treeNDormCounts[player][1] + turn.treeNDormCounts[player][0] >= 2) &&
                            (turn.treeNDormCounts[player][1] == 0 || tmp_suns < (short)treeBaseCost[2] + turn.treeCounts[player][2] - 1) &&
                            (turn.treeNDormCounts[player][0] == 0 || tmp_suns < (short)treeBaseCost[1] + turn.treeCounts[player][1])) {
                            if (
                                (turn.treeNDormCounts[player][1] >= 2 && turn.suns[player] >= ((short)treeBaseCost[2] + turn.treeCounts[player][2]) * 2 + 1) ||
                                (turn.treeNDormCounts[player][1] == 1 && turn.treeNDormCounts[player][0] >= 1 && turn.suns[player] >= (short)treeBaseCost[2] + turn.treeCounts[player][2] + (short)treeBaseCost[1] + turn.treeCounts[player][1] - 1) ||
                                (turn.treeNDormCounts[player][0] >= 2 && turn.suns[player] >= ((short)treeBaseCost[1] + turn.treeCounts[player][1]) * 2 + 1)
                                ) {
                                cerr << "Preferred to GROW 2 small trees than only 1 big tree" << endl;
                                continue;
                            }
                        }
                    // }*/
                    // DONE: do not grow the center too soon (estimating better sun earned on the external border)
                    for (char i = 0; i < turn.trees.size(); ++i) {
                        const Tree& tree = turn.trees[i];
                        if (tree.owner == player && !tree.isDormant && tree.size == sz - 1) {
                            double value = (((double)cells[tree.cellIndex].richness - 1) * 2 + turn.nutrients) * 3 * ((double)turn.day / DAYS) + ((double)tree.size + 1) * countTreeInSun(turn, tree.cellIndex, tree.size + 1, 1, DAYS, 0.9);
                             if (value > maxValue) {
                                maxValue = value;
                                maxCellIdx = tree.cellIndex;
                            }
                        }
                    }
                    if (maxValue > 0) {
                        return Action(GROW, maxCellIdx);
                    }
                }
            }
        }
    }

    if (turn.day >= 2 && turn.suns[player] >= turn.treeCounts[player][0] && ((3-0 < DAYS-turn.day-1 && !(sellItAll && turn.treeCounts[player][3] + turn.treeCounts[player][2] + turn.treeCounts[player][1] > 0)) || turn.treeCounts[player][0] == 0)) {
        // Generate the best SEED action
        char maxSeedCellIdx = 0;
        for (char i = 0; i < turn.trees.size(); ++i) {
            const Tree& tree = turn.trees[i];
            if (tree.owner == player && !tree.isDormant && tree.size > 0) {
                const Cell& treeCell = cells[tree.cellIndex];
                // iterate on destination cells
                char sz = tree.size;
                for (char j = (sz+1)*sz*3 - 1; j >= 0; --j) {
                    const Cell& cell = cells[treeCell.neighs[j]];
                    if (cell.richness > 0) {
                        if (turn.cellToTree[cell.index] == -1) {
                            double value = (((double)cell.richness - 1) * 2 + turn.nutrients) * 3 + countTreeInSun(turn, cell.index, 1, 2, 6, 0.9);
                            if (value > maxValue) {
                                maxValue = value;
                                maxCellIdx = treeCell.index;
                                maxSeedCellIdx = cell.index;
                            }
                        }
                    }
                }
            }
        }
        if (maxValue > 0) {
            return Action(SEED, maxCellIdx, maxSeedCellIdx);
        }
    }
    return Action(WAIT);
}

int main() {
    cin >> cells;
#ifndef NDEBUG
    cerr << cells << endl;
#endif
    // game loop
    while (1) {
        Turn turn;
        cin >> turn;
#ifndef NDEBUG
        cerr << turn << endl;
#endif

        Action myAction = getNextAction(turn, ME);

        // GROW cellIdx | SEED sourceIdx targetIdx | COMPLETE cellIdx | WAIT <message>
        cout << myAction << endl;
    }
}
