/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"
#include <functional>

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    reachablePaths.clear();
    while (!callStack.empty())
        callStack.pop();

    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
        for (auto snk : sinks)
        {
            // Track states on the current path, rather than globally: different
            // branches and different callers may legitimately share a node.
            // Enumerate paths without repeating a (node, call stack) state.
            using State = pair<unsigned, stack<unsigned>>;
            set<State> active;
            vector<unsigned> path;
            function<void(const ICFGNode *)> dfs = [&](const ICFGNode *node)
            {
                const State state(node->getId(), callStack);
                if (!active.insert(state).second)
                    return;

                path.push_back(node->getId());
                if (node->getId() == snk)
                {
                    if (callStack.empty())
                        recordPath(path);
                }
                else
                {
                    for (const auto *edge : node->getOutEdges())
                    {
                        if (const auto *call = SVFUtil::dyn_cast<CallCFGEdge>(edge))
                        {
                            callStack.push(call->getCallSite()->getId());
                            dfs(edge->getDstNode());
                            callStack.pop();
                        }
                        else if (const auto *ret = SVFUtil::dyn_cast<RetCFGEdge>(edge))
                        {
                            const unsigned caller = ret->getCallSite()->getId();
                            if (!callStack.empty() && callStack.top() == caller)
                            {
                                callStack.pop();
                                dfs(edge->getDstNode());
                                callStack.push(caller);
                            }
                        }
                        else if (SVFUtil::isa<IntraCFGEdge>(edge))
                            dfs(edge->getDstNode());
                    }
                }
                path.pop_back();
                active.erase(state);
            };
            dfs(icfg->getICFGNode(src));
        }
}
