#include "graph/smpls.h"
#include "algebra/mptype.h"
#include <memory>

using namespace FSM;

using ELSEdge = ::FSM::Labeled::Edge<CId, CString>;
using ELSState = ::FSM::Labeled::State<CId, CString>;
using ELSSetOfStates = ::FSM::Labeled::SetOfStates<CId, CString>;
using ELSSetOfEdges = ::FSM::Abstract::SetOfEdges;
using ELSSetOfEdgeRefs = ::FSM::Abstract::SetOfEdgeRefs;
using ELSSetOfStateRefs = ::FSM::Abstract::SetOfStateRefs;

namespace FSM {

    // destructor is put deliberately into the cc source to ensure the class vtable is accessible
    // see: <https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable>

	EdgeLabeledScenarioFSM::~EdgeLabeledScenarioFSM(){}

    /*removes states of the fsm with no outgoing edges.*/
	// TODO (Marc Geilen) carefully check and test and move to fsm
    void EdgeLabeledScenarioFSM::removeDanglingStates()
    {

        // temporary sets of states and edges
        ELSSetOfEdgeRefs edgesToBeRemoved;
        ELSSetOfStateRefs statesToBeRemoved;

        ELSSetOfStates& elsStates = this->getStates();
        auto& elsEdges = dynamic_cast<ELSSetOfEdges&>(this->getEdges());


        /*go through all edges and find all edges that end in
        dangling states. Also store dangling states.*/
        for (const auto& it: elsEdges)
        {

            auto& e = dynamic_cast<ELSEdge&>(*it.second);
            const auto& s = dynamic_cast<const ELSState&>(e.getDestination());
            const auto& oEdges = dynamic_cast<const ELSSetOfEdges&>(s.getOutgoingEdges());
            if (oEdges.empty())
            {
                edgesToBeRemoved.insert(&e);
                statesToBeRemoved.insert(&s);
            }
        }

        while (edgesToBeRemoved.size() != 0)
        {

            //remove dangling states
            for (const auto *srIt: statesToBeRemoved)
            {
				this->removeState(dynamic_cast<const ELSState&>(*srIt));
                delete srIt;
            }

            //remove edges ending in dangling states
            //remove edges ending in dangling states from the outgoing edges of their source states
            for (auto *erIt: edgesToBeRemoved) {
                const auto& e = dynamic_cast<const ELSEdge&>(*erIt);
                this->removeEdge(e);
				const auto& s = dynamic_cast<const ELSState&>(e.getSource());


				// TODO (Marc Geilen) redesign for opportunities for subclasses to modify their FSM structure while external users get const access only. Perhaps through protected access.
				// Maybe request modifiable access to a given const state/edge?
				auto& ms = const_cast<ELSState&>(s);
                ms.removeOutgoingEdge(e);
            }

            //empty the temporary sets
            edgesToBeRemoved.clear();
            statesToBeRemoved.clear();

            elsStates = this->getStates();
            elsEdges = this->getEdges();

            /*go through all edges and find all edges that end in
            dangling states. Also store dangling states.*/
            for (const auto& it: elsEdges)
            {

                auto& e = dynamic_cast<ELSEdge&>(*(it.second));
                const auto& s = dynamic_cast<const ELSState&>(e.getDestination());
                const auto& oEdges = dynamic_cast<const ELSSetOfEdges&>(s.getOutgoingEdges());
                if (oEdges.empty())
                {
                    edgesToBeRemoved.insert(&e);
                    statesToBeRemoved.insert(&s);
                }
            }
        }
    }

}



namespace MaxPlus
{

	/**
	 * returns the largest finite element of a row up to and including colNumber
	 */
	MPTime getMaxOfRowUntilCol(Matrix& M, uint rowNumber, uint colNumber)
	{
		if (rowNumber > M.getRows()) {
			throw CException("Matrix getMaxOfRow input row index out of bounds.");
		}
		if (colNumber > M.getCols()) {
			throw CException("Matrix getMaxOfRowUntilCol input col index out of bounds.");
		}
    	MPTime largestEl = MP_MINUSINFINITY;
		for (unsigned int c = 0; c < colNumber; c++) {
			largestEl = MP_MAX(largestEl, M.get(rowNumber, c));
		}
		return largestEl;
	}

	/**
	 * returns the largest finite element of a row up to and including colNumber
	 */
	MPTime getMaxOfColUntilRow(Matrix& M, uint colNumber, uint rowNumber)
	{
		if (rowNumber > M.getRows()) {
			throw CException("Matrix getMaxOfRow input row index out of bounds.");
		}
		if (colNumber > M.getCols()) {
			throw CException("Matrix getMaxOfRowUntilCol input col index out of bounds.");
		}
    	MPTime largestEl = MP_MINUSINFINITY;
		for (unsigned int r = 0; r < rowNumber; r++) {
			largestEl = MP_MAX(largestEl, M.get(r, colNumber));
		}
		return largestEl;
	}


	/*
	* Loads an entity from its starting '{' to the corresponding '}'
	*/
	std::string loadEntity(std::ifstream& stream, std::string line)
	{
		std::string out = line;
		size_t openBracketCount = 0;
		size_t closeBracketCount = 0;
		openBracketCount += std::count(line.begin(), line.end(), '{');
		closeBracketCount += std::count(line.begin(), line.end(), '}');

		// get the whole activity {} into a string
		while (getline(stream, line))
		{

			openBracketCount += std::count(line.begin(), line.end(), '{');
			closeBracketCount += std::count(line.begin(), line.end(), '}');
			out += line;

			if (openBracketCount == closeBracketCount)
			{
				openBracketCount = 0;
				closeBracketCount = 0;
				break;
			}
		}
		return out;
	}
	std::shared_ptr<MaxPlusAutomaton> SMPLS::convertToMaxPlusAutomaton() const
	{
		auto mpa = std::make_shared<MaxPlusAutomaton>();
		//transposeMatrices();
		// create the FSM states for every pair of a state of the FSMSADF's FSM
		// and an initial token

		const auto& I = dynamic_cast<const ELSSetOfStates&>(elsFSM->getInitialStates());
		const auto& F = dynamic_cast<const ELSSetOfStates&>(elsFSM->getFinalStates());
		auto& Q = dynamic_cast<ELSSetOfStates&>(elsFSM->getStates());
		for (const auto& q: Q)
		{
            auto& qq = *(q.second);
			const auto& e = qq.getOutgoingEdges();
			unsigned int nrTokens = 0;
			if (! e.empty())
			{
				CString label = (dynamic_cast<ELSEdge*>(*e.begin()))->getLabel();

				for (const auto& smIt: *sm)
				{
					if ((smIt).first == label)
					{
						nrTokens = (smIt).second->getCols();
						break;
					}
				}
			}
			else
			{
				// TODO(mgeilen): check what is going on
				// this is a bit iffy, but I dont have time to make it solid now
				nrTokens = (*sm->begin()).second->getCols();
			}
			// create a state for (q, k)
			CId qId = (dynamic_cast<ELSState&>(qq)).getLabel();
			bool isInitial = false;
			bool isFinal = false;
			// if els state is initial
			for (const auto& i: I)
			{
				CId iId = (dynamic_cast<ELSState&>(*(i.second))).getLabel();
				if (iId == qId)
				{
					isInitial = true;
					break;
				}
			}
			// if els state is final
			for (const auto& f: F)
			{
				CId fId = (dynamic_cast<ELSState&>(*(f.second))).getLabel();
				if (fId == qId)
				{
					isFinal = true;
					break;
				}
			}

			// create the states needed per transition matrix
			for (unsigned int k = 0; k < nrTokens; k++)
			{
				MPAState* s = mpa->addState(makeMPAStateLabel(qId, k));
				//std::cout << "DEBUG adding state to mpa id: " << (CString)(s->getLabel().id) << ", tkn: " <<  (s->getLabel().tokenNr)<< std::endl;
				if (isInitial)
					mpa->addInitialState(*s);
				if (isFinal)
					mpa->addFinalState(*s);
			}
		}
		// add the edges. For every edge (q1, q2) in the original fsm,
		// let s be the scenario of q2 and let M_s be the matrix of s
		// For every non -inf element d=M_s(k,m) add an edge from state (q1,k)
		// to (q2,m) labelled with d.

		// for every state of the fsm...
		for (const auto& q: Q)
		{
			auto& q1 = dynamic_cast<ELSState&>(*(q.second));
			CId q1Id = q1.getLabel();

			// for every outgoing edge of the state
			const auto& t = dynamic_cast<const ELSSetOfEdgeRefs&>(q1.getOutgoingEdges());
			for (auto *e: t)
			{
				auto& tr = dynamic_cast<ELSEdge&>(*e);
				CId q2Id = dynamic_cast<const ELSState&>(tr.getDestination()).getLabel();
				CString sc = tr.getLabel();
				std::shared_ptr<Matrix> Ms = (*sm)[sc];
				size_t r = Ms->getRows();
				size_t c = Ms->getCols();

				// for every entry in the scenario matrix of that edge
				for (size_t row = 0; row < r; row++)
				{
					for (size_t col = 0; col < c; col++)
					{
						MPDelay d = Ms->get(static_cast<unsigned int>(row), static_cast<unsigned int>(col));

						if (!MP_ISMINUSINFINITY(d))
						{
							MPAState& src = mpa->getStateLabeled(makeMPAStateLabel(q1Id, static_cast<unsigned int>(col)));
							MPAState& dst = mpa->getStateLabeled(makeMPAStateLabel(q2Id, static_cast<unsigned int>(row)));
							MPAEdgeLabel el = makeMPAEdgeLabel(d, sc);
							el.scenario = CString(tr.getLabel());
							mpa->addEdge(src, el, dst);
						}
					}
				}
			}
		}

		return mpa;
	}

	/* this is not fully functional for now. you can use the version that it prints out
	* it also does not check for input actions that don't belong to the same event, which it should.
	*/
	void SMPLSwithEvents::saveDeterminizedIOAtoFile(const CString& file)
	{
		/**
		 * Deterministic IOA is defined with:
		 *	The I/O automaton has exactly one initial state;
		 *  The final states have no outgoing transition;
		 *	Branches in the I/O automaton are based on different outcomes of the same event;
		 */

		if (this->ioa == nullptr) {
			throw CException("The automaton of smpls is not loaded!");
		}

		std::ofstream outfile(file);
		outfile << "ioautomaton statespace{ \r\n";
		const auto& I = dynamic_cast<const IOASetOfStates&>(this->ioa->getInitialStates());

		const auto& finalStates =dynamic_cast<const IOASetOfStates&>(this->ioa->getFinalStates());

		CString errMsg = "";
		auto i = I.begin();
		auto& s = dynamic_cast<IOAState&>(*((*i).second));
		i++;
		//we remove the rest of the initial states since only one is allowed
		for (; i != I.end();)
		{
			ioa->removeState(dynamic_cast<const IOAState&>(*((*i).second)));
		}
		IOASetOfStateRefs visitedStates;
		determinizeUtil(s, visitedStates, finalStates, &errMsg, outfile);
		outfile << "}";
		outfile.close();
	}

	/**
	* checks the consistency rules for SMPLS with events
	*/

	bool SMPLSwithEvents::isConsistent()
	{
		if (this->ioa == nullptr) {
			throw CException("The automaton of smpls is not loaded!");
		}
		std::list<Event> eventList;

		const auto& I = dynamic_cast<const IOASetOfStates&>(this->ioa->getInitialStates());

		const auto& finalStates = dynamic_cast<const IOASetOfStates&>(this->ioa->getFinalStates());

		std::map<const IOAState*, std::list<Event>*> visited;
		CString errMsg = "";
		for (const auto& i: I)
		{
			isConsistentUtil((dynamic_cast<IOAState&>(*(i.second))), eventList, finalStates, &errMsg, visited);
		}

		if (errMsg != "")
		{
			// TODO(mgeilen) no std:cout
			std::cout << "IO Automaton is not consistent, error message: " << std::endl;
			std::cout << errMsg << std::endl;
			return false;
		}

		return true;
	}

	/**
		 * creates a max-plus automaton from SMPLS with events
		 */
	std::shared_ptr<MaxPlusAutomaton> SMPLSwithEvents::convertToMaxPlusAutomaton()
	{
		dissectScenarioMatrices();
		IOASetOfEdgeRefs visitedEdges;
		std::multiset<Event> eventList;

		// create the elsFsm with the same state structure but we change the matrices in a depth-first search
		const IOASetOfStates& I = dynamic_cast<IOASetOfStates&>(this->ioa->getStates());
		for (const auto& i: I)
		{
			this->elsFSM->addState(dynamic_cast<ELSState&>(*(i.second)).getLabel());
		}

		const auto& I2 = dynamic_cast<const IOASetOfStates&>(this->ioa->getInitialStates());
		for (const auto& i:  I2)
		{
			prepareMatrices((dynamic_cast<IOAState&>(*(i.second))), eventList, visitedEdges);
			this->elsFSM->addInitialState((dynamic_cast<ELSState&>(*(i.second))));
		}
		const auto& I3 = dynamic_cast<const IOASetOfStates&>(this->ioa->getFinalStates());
		for (const auto& i: I3)
		{
			this->elsFSM->addFinalState((dynamic_cast<ELSState&>(*(i.second))));
		}
		return SMPLS::convertToMaxPlusAutomaton();
	}

	/*
		* after we generate the matrices through 'prepareMatrices', we have to make them
		* all square (to the size of the biggest matrix) by adding rows and cols of -infty
		*/
	void SMPLSwithEvents::makeMatricesSquare()
	{
		for (const auto& it: *sm) {
			std::shared_ptr<Matrix> m = it.second;
			m->addCols(biggestMatrixSize - m->getCols());
			m->addRows(biggestMatrixSize - m->getRows());
		}
	}

	// transposes all matrices of the SMPLS
	void SMPLS::transposeMatrices()
	{
		for (auto& it: *this->sm)
		{
			it.second = it.second->getTransposedCopy();
		}
	}
	/*
     * goes through the gamma relation and finds the event of the outcome
	 */
	Event SMPLSwithEvents::findEventByOutcome(const EventOutcome& outcome) const
	{
		for (const auto& gammaIterator: *gamma)
		{
			// finding the event in gamma
			if ((std::pair<Event, EventOutcome>(gammaIterator)).second == outcome)
			{
				return (std::pair<Event, EventOutcome>(gammaIterator)).first;
			}
		}
		throw CException("Event of outcome " + outcome + " not found!");
	}

	/*
     * goes through the sigma relation and finds the event emitted by mode
	 */
	Event SMPLSwithEvents::findEventByMode(const Mode& mode) const
	{
		//find the event name ...
		for (const auto& sigmaIterator: *sigma)
		{
			if ((std::pair<Mode, Event>(sigmaIterator)).first == mode)
			{
				return (std::pair<Mode, Event>(sigmaIterator)).second;
			}
		}
		throw CException("Event of mode " + mode + " not found!");
	}

	/**
	 * smpls with events needs to prepare the matrices to be able to perform analysis.
	 * this includes adding rows and columns of -inf and 0 based on the spec
	 * allowing the system to analyze processing or conveying event timings
	 */
	void SMPLSwithEvents::prepareMatrices(const IOAState& s, std::multiset<Event>& eventList, IOASetOfEdgeRefs& visitedEdges)
	{
		const auto& adj = dynamic_cast<const IOASetOfEdgeRefs&>(s.getOutgoingEdges());
		for (auto *i: adj)
		{
			auto& e = dynamic_cast<IOAEdge&>(*i);
			if (visitedEdges.count(&e) > 0) {
				continue;
			}

			//create a unique label for the new edge. this name will also be the scenario name for sm
			CString scenarioName = CString(s.getId());
			scenarioName += ",";
			scenarioName += e.getLabel().first + "," + e.getLabel().second;

			// add the edge with unique name between the corresponding states
			elsFSM->addEdge(elsFSM->getStateLabeled(s.getLabel()), scenarioName, elsFSM->getStateLabeled((dynamic_cast<const IOAState&>(e.getDestination())).getLabel()));

			// make a copy so that child node can not modify the parent nodes list of events
			// only adds and removes and passes it to it's children
			std::multiset<Event> eList(eventList);

			OutputAction output = e.getLabel().second;
			InputAction input = e.getLabel().first;

			auto disSm = std::make_shared<DissectedScenarioMatrix>();
			std::shared_ptr<Matrix> sMatrix; //matrix to be generated and assigned to this edge
			CString outputActionEventName = "";
			int emittingEventIndex = -1;
			int processingEventIndex = -1;
			int eventIndexCounter = 0;
			if (output != "")
			{
				// find the dissected matrix structure needed for this edge
				disSm = findDissectedScenarioMatrix(output);
				if (disSm == nullptr){
					throw CException("scenario " + output + " not found in dissected matrices!");
				}

				sMatrix = disSm->core.begin()->second->createCopy();

				// look if it has any events to emit
				if (!disSm->eventRows.empty())
				{
					outputActionEventName = findEventByMode(output);

					// ... and where it would sit in the sorted event std::multiset
					std::multiset<Event> eTempList(eList);
					eTempList.insert(outputActionEventName); // we insert in a copy to see where it would sit
					emittingEventIndex = static_cast<int>(eTempList.size() - 1);
					auto it = eTempList.rbegin();
					for (; it != eTempList.rend(); it++)
					{
						if (*it == outputActionEventName)
						{
							break;
						}
						emittingEventIndex--;
					}
				}
			}
			else
			{
				sMatrix = std::make_shared<Matrix>(numberOfResources, numberOfResources, MatrixFill::Identity);
			}
			if (input != "") // find the index of the event to be removed from the list
			{
				Event e = findEventByOutcome(input);
				//we have to find the first occurrence from top
				processingEventIndex = static_cast<int>(distance(eList.begin(), eList.find(e)));
			}

			if (! eList.empty())
			{
				CString eventToBeRemoved;
				std::multiset<Event> eTempList(eList);
				std::multiset<Event>::iterator eventIter;
				for (eventIter = eTempList.begin(); eventIter != eTempList.end(); eventIter++)
				{
					sMatrix->addCols(1);

					// this means we have an event in the list which is being conveyed to the next matrix
					// so we add the necessary rows and cols to facilitate that

					if (eventIndexCounter == processingEventIndex) //we add B_c and B_e
					{
						uint cols = sMatrix->getCols();
						uint y = 0;
						for (; y < numberOfResources; y++) // B_c (refer to paper)
						{
							sMatrix->put(y, cols - 1, sMatrix->getMaxOfRow(y));
						}
						for (; y < numberOfResources; y++) // B_e (refer to paper)
						{
							sMatrix->put(y, cols - 1, getMaxOfRowUntilCol(*sMatrix, y, numberOfResources));
						}

						eList.erase(eList.lower_bound(*eventIter)); // event is processed, remove from eList
					}
					else // or are we conveying this event to the next state vector? I_c(q) (refer to paper)
					{
						sMatrix->addRows(1);											 // we already added a col of -infty, now we add the row
						sMatrix->put(sMatrix->getRows() - 1, sMatrix->getCols() - 1, MPTime(0)); // and 0 in the intersection of that row and col
					}
					// if we need to add the event row at the bottom
					if ((eventIndexCounter == emittingEventIndex) ||
						(eventIndexCounter + 1 == static_cast<int>(eTempList.size()) && emittingEventIndex == eventIndexCounter + 1)) // the second condition happens when we have reached the end of event list but we still need to add the event at the bottom
					{
						sMatrix->addRows(1);
						std::shared_ptr<Matrix> eventRow = *disSm->eventRows.begin(); // for now we just assume max one event emitted per mode
						uint cols = eventRow->getCols();
						uint x = 0;
						for (; x < cols; x++) // A_e (refer to paper)
						{
							sMatrix->put(sMatrix->getRows() - 1, x, eventRow->get(0, x));
						}
						for (; x < sMatrix->getCols(); x++) // B_e (refer to paper), this happens when we add B_c before adding A_e, so we have to make sure B_e that's added here is max of row
						{
							if (getMaxOfColUntilRow(*sMatrix, x, numberOfResources) > MP_MINUSINFINITY) {
								// we must only add B_e under cols that correspond to B_c
								sMatrix->put(sMatrix->getRows() - 1, x, getMaxOfRowUntilCol(*eventRow, 0, numberOfResources));
							}
						}
					}

					eventIndexCounter++;
				}
			}
			else if (output != "") {
				if (!disSm->eventRows.empty())
				{
					sMatrix->addRows(1);
					std::shared_ptr<Matrix> eventRow = *disSm->eventRows.begin();
					uint cols = eventRow->getCols(); // for now we just assume max one event emitted per mode

					for (uint x = 0; x < cols; x++)
					{
						sMatrix->put(sMatrix->getRows() - 1, x, eventRow->get(0, x));
					}
				}
}
			// we store the events emitted by modes as we move along the automaton
			if (outputActionEventName != "")
			{
				eList.insert(outputActionEventName);
			}

			(*sm)[scenarioName] = sMatrix;
			visitedEdges.insert(&e);

			// we need this later to make all matrices square
			biggestMatrixSize = std::max(biggestMatrixSize, std::max(sMatrix->getCols(), sMatrix->getRows()));

			prepareMatrices(dynamic_cast<const IOAState&>(e.getDestination()), eList, visitedEdges);
		}
	}

	std::shared_ptr<DissectedScenarioMatrix> SMPLSwithEvents::findDissectedScenarioMatrix(const CString& sName)
	{
		std::shared_ptr<DissectedScenarioMatrix> dis = nullptr;
		for (const auto& i: *disMatrices) {
			if (i->core.begin()->first == sName)
			{
				dis = i;
				break;
			}
		}
		return dis;
	}

	/**
		 * recursive part of isConsistent
		 */
	void SMPLSwithEvents::isConsistentUtil(const IOAState& s, std::list<Event>& eventList, const IOASetOfStates& finalStates, CString *errMsg, std::map<const IOAState*, std::list<Event> *> &visited)
	{
		auto it = visited.find(&s);
		if (it != visited.end()) // we have already visited this state but we must check for inconsistency before leaving the state
		{
			if (!compareEventLists(*it->second, eventList))
			{
				*errMsg = CString("Different paths leading to different events at state " + std::to_string(s.getLabel()));
			}
			// we have checked for inconsistency, no need to go further down this state
			return;
		}
		visited.emplace(&s, &eventList);

		if (finalStates.count(s.getLabel())>0)
		{
			if (eventList.size() > 0)
			{
				*errMsg = "Event " + (*eventList.begin()) + " has not been processed by the end of the word.\r\n";
				return;
			}
		}
		else // If current state is not final
		{
			const auto& adj = dynamic_cast<const IOASetOfEdgeRefs&>(s.getOutgoingEdges());
			for (auto *i: adj)
			{
				// make a copy so that child node can not modify the parent nodes list
				// only adds and removes and passes it to its children
				std::list<Event> eList(eventList);

				auto& e = dynamic_cast<IOAEdge&>(*i);
				OutputAction output = e.getLabel().second;
				InputAction input = e.getLabel().first;
				if (input != "")
				{
					bool eventFound = false;
					std::list<Event>::iterator eventIter;
					for (eventIter = eList.begin(); eventIter != eList.end(); eventIter++)
					{
						std::list<std::pair<Event, EventOutcome>>::iterator gammaIterator;
						for (gammaIterator = gamma->begin(); gammaIterator != gamma->end(); gammaIterator++)
						{
							auto p = (std::pair<Event, EventOutcome>(* gammaIterator));
							if (p.first == *eventIter) // finding the event in gamma
							{
								if (p.second == input)
								{
									eList.erase(eventIter); // event is processed, remove from eventList
									eventFound = true;
									break;
								}
							}
						}
						if (eventFound) {
							break;
						}
					}
					if (!eventFound)
					{
						// we are processing a non-emitted event
						*errMsg = CString("on edge: " + std::to_string(s.getLabel()) + " - " + std::string(input + "," + output + ", Processing event outcome : " + input + " where the event is not emitted yet.\r\n"));
						return;
					}
				}
				// we store the events emitted by modes as we move along the automaton
				if (output != "")
				{
					// look if it has any events to emit
					std::list<std::pair<Mode, Event>>::iterator sigmaIterator;
					for (sigmaIterator = sigma->begin(); sigmaIterator != sigma->end(); sigmaIterator++)
					{
						if ((std::pair<Mode, Event>(* sigmaIterator)).first == output) // this output action emits an event
						{
							auto a = (std::pair<Mode, Event>(* sigmaIterator));
							eList.push_back(a.second);
						}
					}
				}
				const auto& s2 = dynamic_cast<const IOAState&>(e.getDestination());

				isConsistentUtil(s2, eList, finalStates, errMsg, visited);
			}
		}
	}

	/**
		 * recursive part of determinize
		 */
	void SMPLSwithEvents::determinizeUtil(const IOAState& s, IOASetOfStateRefs& visited, const IOASetOfStates& finalStates, CString* errMsg, std::ofstream& outfile)
	{
		/**
		 * Deterministic IOA is defined with:
		 *	exactly one initial state;
		 *  final states that have no outgoing transition;
		 *	and branches only based on different outcomes of the same event;
		 */
		 // implementation is incomplete yet. we have to remove unreachable states and their edges after this
		if (visited.count(&s)>0) {
			return;
		}

		visited.insert(&s);

		const auto& outEdge = dynamic_cast<const IOASetOfEdgeRefs&>(s.getOutgoingEdges());
		auto i = outEdge.begin();

		const auto* e = dynamic_cast<const IOAEdge*>(*i);
		InputAction input = e->getLabel().first;
		if (input == "")
		{
			const auto& s2 = dynamic_cast<const IOAState&>(e->getDestination());
			outfile << s.stateLabel << "-," << e->getLabel().second << "->" << s2.stateLabel;
			ioa->removeEdge(*e);

			i++;
			//we go through all the rest of the edges and remove them from the automaton
			for (; i != outEdge.end(); ++i)
			{
				e = dynamic_cast<IOAEdge*>(*i);
				ioa->removeEdge(*e);
				// since we only start with one initial state and remove the others, then the destination state of this edge will be unreachable
				//ioa->removeState(dynamic_cast<IOAState*>(e->getDestination()));
			}

			if (finalStates.count(s2.getId())>0)
			{
				outfile << " f\n";
			}
			else {
				outfile << "\n";
				determinizeUtil(s2, visited, finalStates, errMsg, outfile);
			}
		}
		else // we have an input action
		{
			Event ev = this->findEventByOutcome(input);
			//we go through all the edges
			for (; i != outEdge.end(); i++)
			{
				e = dynamic_cast<const IOAEdge*>(*i);
				input = e->getLabel().first;
				// we remove the ones that dont have an input action from the automaton
				if (input == "")
				{
					ioa->removeEdge(*e);
					// since we only start with one initial state and remove the others, then the destination state of this edge will be unreachable
					//ioa->removeState(dynamic_cast<IOAState*>(e->getDestination()));
				}
				else
				{
					// only allow edges with the outcome of the same event
					if (this->findEventByOutcome(input) == ev)
					{
						const auto& s2 = dynamic_cast<const IOAState&>(e->getDestination());
						outfile << s.stateLabel << "-" << e->getLabel().first << "," << e->getLabel().second << "->" << s2.stateLabel;

						ioa->removeEdge(*e);
						if (finalStates.count(s2.getId())>0)
						{
							outfile << " f\n";
						}
						else {
							outfile << "\n";
							determinizeUtil(s2, visited, finalStates, errMsg, outfile);
						}
					}
					else
					{
						ioa->removeEdge(*e);
					}

				}
			}

		}

	}
	bool SMPLSwithEvents::compareEventLists(std::list<Event>& l1, std::list<Event>& l2)
	{
		bool result = true;

		if (l1.size() != l2.size())
		{
			return false;
		}

		auto it2 = l1.begin();
		for (; it2 != l1.end(); it2++)
		{
			result = false;
			auto it3 = l2.begin();
			for (; it3 != l2.end(); it3++)
			{
				if (*it2 == *it3)
				{
					result = true;
					break;
				}
			}
		}
		return result;
	}

	void SMPLSwithEvents::dissectScenarioMatrices()
	{
		ScenarioMatrices::iterator s;
		for (s = sm->begin(); s != sm->end(); s++)
		{
			numberOfResources = 0;
			std::shared_ptr<DissectedScenarioMatrix> dis = std::make_shared<DissectedScenarioMatrix>();
			dis->core.insert(*s);

			std::shared_ptr<Matrix> m = s->second; //->getTransposedCopy();
			std::list<uint> mSubIndices;
			numberOfResources = std::min(m->getRows(), m->getCols());
			for (uint x = 0; x < numberOfResources; x++)
			{
				mSubIndices.push_back(x);
				//numberOfResources++;
			}

			dis->core.begin()->second = m->getSubMatrixNonSquareRowsPtr(mSubIndices);
			if (m->getRows() > numberOfResources) // if we have event rows
			{
				mSubIndices.clear();
				for (uint x = numberOfResources; x < std::max(m->getRows(), m->getCols()); x++)
				{
					mSubIndices.push_back(x);
					std::shared_ptr<Matrix> mEventRow = m->getSubMatrixNonSquareRowsPtr(mSubIndices);
					dis->eventRows.push_back(mEventRow);
					mSubIndices.clear();
				}
			}
			disMatrices->push_back(dis);
		}
	}

	// void SMPLS::loadAutomatonFromIOAFile(CString fileName)
	// {
	// 	std::cout << "Loading automaton." << std::endl;
	// 	// remember which state maps to which new state
	// 	std::map<CId, CString> stateMap;
	// 	elsFSM = new EdgeLabeledScenarioFSM();

	// 	std::ifstream input_stream(fileName);
	// 	if (!input_stream)
	// 		std::cerr << "Can't open input file : " << fileName << std::endl;
	// 	std::string line;

	// 	CId stateId = 0;
	// 	// extract all the text from the input file
	// 	while (getline(input_stream, line))
	// 	{

	// 		size_t i1 = line.find("-");
	// 		size_t i2 = line.find("->");

	// 		if (i1 != std::string::npos && i2 != std::string::npos)
	// 		{
	// 			if (i1 < i2) // if we have a valid line as in "state i f --edge--> state2 f"
	// 			{
	// 				CString srcState = "";
	// 				CString edge = "";
	// 				CString destState = "";

	// 				CString srcStateDescription = CString(line.substr(0, i1));
	// 				srcStateDescription.trim();
	// 				srcState = CString(srcStateDescription.substr(0, srcStateDescription.find(" ")));

	// 				CString destStateDescription = CString(line.substr(i2 + 2));
	// 				destStateDescription.trim();
	// 				destState = CString(destStateDescription.substr(0, destStateDescription.find_first_of(" ")));

	// 				std::string edgeDescription = line.substr(i1, i2 - i1);
	// 				edge = CString(edgeDescription.substr(edgeDescription.find_first_not_of("-")));
	// 				edge = CString(edge.substr(0, edge.find("-")));
	// 				edge.trim();

	// 				// look for invalid edge desc
	// 				size_t tempIndex1 = edge.find(";");
	// 				size_t tempIndex2 = edge.find(",");

	// 				if (tempIndex1 != std::string::npos || tempIndex2 != std::string::npos) // we have an ioa
	// 					throw CException("invalid edge description for normal automaton. edge can not contain ';' or ','");

	// 				// look if the state already exists
	// 				bool duplicateState = false;
	// 				ELSState* q1 = nullptr;
	// 				for (unsigned int i = 0; i < stateMap.size(); i++)
	// 				{
	// 					if (stateMap[i] == srcState)
	// 					{
	// 						duplicateState = true;
	// 						delete q1;
	// 						q1 = &(elsFSM->getStateLabeled(i));
	// 						break;
	// 					}
	// 				}

	// 				if (!duplicateState)
	// 				{
	// 					q1 = elsFSM->addState(stateId);
	// 					stateMap[stateId] = srcState;
	// 					stateId++;
	// 				}
	// 				// look for state annotations
	// 				srcStateDescription.erase(0, srcState.length());
	// 				if (srcStateDescription.find(" i") != std::string::npos || srcStateDescription.find(" initial") != std::string::npos)
	// 					elsFSM->addInitialState(*q1);
	// 				if (srcStateDescription.find(" f") != std::string::npos || srcStateDescription.find(" final") != std::string::npos)
	// 					elsFSM->addFinalState(*q1);

	// 				duplicateState = false;
	// 				ELSState* q2 = nullptr;
	// 				for (unsigned int i = 0; i < stateMap.size(); i++)
	// 				{
	// 					if (stateMap[i] == destState)
	// 					{
	// 						duplicateState = true;

	// 						q2 = &(elsFSM->getStateLabeled(i));
	// 						break;
	// 					}
	// 				}

	// 				if (!duplicateState)
	// 				{
	// 					q2 = elsFSM->addState(stateId);
	// 					stateMap[stateId] = destState;
	// 					stateId++;
	// 				}
	// 				// look for state annotations
	// 				destStateDescription.erase(0, destState.length());
	// 				if (destStateDescription.find(" i") != std::string::npos || destStateDescription.find(" initial") != std::string::npos)
	// 					elsFSM->addInitialState(*q2);
	// 				if (destStateDescription.find(" f") != std::string::npos || destStateDescription.find(" final") != std::string::npos)
	// 					elsFSM->addFinalState(*q2);

	// 				elsFSM->addEdge(*q1, edge, *q2);
	// 			}
	// 			else
	// 			{
	// 				throw CException("invalid line in ioa file!");
	// 			}
	// 		}
	// 	}
	// 	std::cout << std::endl
	// 		<< "automaton states size: " << elsFSM->getStates().size() << std::endl;
	// 	std::cout << "automaton transitions size: " << elsFSM->getEdges().size() << std::endl;
	// }

	// void SMPLS::loadAutomatonFromDispatchingFile(CString fileName)
	// {
	// 	std::cout << "Loading dispatching file." << std::endl;
	// 	// remember which state maps to which new state
	// 	std::map<CId, CString> stateMap;
	// 	elsFSM = new EdgeLabeledScenarioFSM();

	// 	std::ifstream input_stream(fileName);
	// 	if (!input_stream)
	// 		std::cerr << "Can't open input file : " << fileName << std::endl;
	// 	CString line;

	// 	int counter = 0;
	// 	while (getline(input_stream, line))
	// 	{
	// 		size_t i1 = line.find("{");
	// 		if (i1 != std::string::npos)
	// 			break;
	// 		if (counter == 10000)
	// 		{
	// 			throw CException("ERROR : Could not find a { which indicates start of dispatching after 10000 lines. are you sure this is a dispatching file?");
	// 		}
	// 		counter++;
	// 	}

	// 	CId stateId = 0;

	// 	int j = 0;
	// 	ELSState* lastState = nullptr;
	// 	// extract all the text from the input file
	// 	while (getline(input_stream, line))
	// 	{
	// 		line.trim();
	// 		if (line.find("}") != std::string::npos)
	// 			break;
	// 		if (line.size() > 0)
	// 		{
	// 			CString srcState = "";
	// 			CString edge = "";
	// 			CString destState = "";

	// 			CString srcStateDescription = "s_" + CString(std::to_string(j));
	// 			srcStateDescription.trim();
	// 			srcState = CString(srcStateDescription.substr(0, srcStateDescription.find(" ")));

	// 			CString destStateDescription = "s_" + CString(std::to_string(1 + j));
	// 			destStateDescription.trim();
	// 			destState = CString(destStateDescription.substr(0, destStateDescription.find_first_of(" ")));

	// 			std::string edgeDescription = line;
	// 			edge = CString(edgeDescription.substr(edgeDescription.find_first_not_of("-")));
	// 			edge = CString(edge.substr(0, edge.find("-")));
	// 			edge.trim();

	// 			std::cout << "Edge: " << edge << std::endl;
	// 			// look if the state already exists
	// 			bool duplicateState = false;
	// 			ELSState* q1 = nullptr;
	// 			for (unsigned int i = 0; i < stateMap.size(); i++)
	// 			{
	// 				if (stateMap[i] == srcState)
	// 				{
	// 					duplicateState = true;
	// 					q1 = &(elsFSM->getStateLabeled(i));
	// 					break;
	// 				}
	// 			}

	// 			if (!duplicateState)
	// 			{
	// 				q1 = elsFSM->addState(stateId);
	// 				stateMap[stateId] = srcState;
	// 				stateId++;
	// 			}

	// 			if (j == 0)
	// 				elsFSM->addInitialState(*q1);

	// 			duplicateState = false;
	// 			ELSState* q2 = nullptr;
	// 			for (unsigned int i = 0; i < stateMap.size(); i++)
	// 			{
	// 				if (stateMap[i] == destState)
	// 				{
	// 					duplicateState = true;

	// 					q2 = &(elsFSM->getStateLabeled(i));
	// 					break;
	// 				}
	// 			}

	// 			if (!duplicateState)
	// 			{
	// 				q2 = elsFSM->addState(stateId);
	// 				stateMap[stateId] = destState;
	// 				stateId++;
	// 			}
	// 			lastState = q2;

	// 			elsFSM->addEdge(*q1, edge, *q2);
	// 			j++;
	// 		}
	// 	}
	// 	elsFSM->addFinalState(*lastState);
	// 	std::cout << std::endl
	// 		<< "automaton states size: " << elsFSM->getStates().size() << std::endl;
	// 	std::cout << "automaton transitions size: " << elsFSM->getEdges().size() << std::endl;
	// }

	// void SMPLSwithEvents::loadIOAutomatonFromDispatchingFile(CString fileName)
	// {
	// 	std::cout << "Loading I/O automaton from dispatching file." << std::endl;
	// 	// remember which state maps to which new state
	// 	std::map<CId, CString> stateMap;
	// 	std::ifstream input_stream(fileName);
	// 	if (!input_stream)
	// 		std::cerr << "Can't open input file : " << fileName << std::endl;
	// 	CString line;
	// 	int lineNumber = 0;
	// 	CId stateId = 0;
	// 	int counter = 0;
	// 	while (getline(input_stream, line))
	// 	{
	// 		size_t i1 = line.find("{");
	// 		if (i1 != std::string::npos)
	// 			break;
	// 		if (counter == 10000)
	// 		{
	// 			throw CException("ERROR : Could not find a '{' which indicates start of dispatching, after 10000 lines. are you sure this is a dispatching file?");
	// 		}
	// 		counter++;
	// 	}
	// 	int j = 0;

	// 	IOAState* lastState = new IOAState(stateId);
	// 	// extract all the text from the input file
	// 	while (getline(input_stream, line))
	// 	{
	// 		lineNumber++;
	// 		CString srcState = "";
	// 		CString edge = "";
	// 		CString destState = "";
	// 		CString inputAction = "";
	// 		CString outputAction = "";
	// 		line.trim();
	// 		if (line.find("}") != std::string::npos)
	// 			break;
	// 		if (line.size() <= 0)
	// 			continue;
	// 		CString srcStateDescription = CString("s_" + std::to_string(j));
	// 		srcStateDescription.trim();
	// 		srcState = CString(srcStateDescription.substr(0, srcStateDescription.find(" ")));

	// 		CString destStateDescription = CString("s_" + std::to_string(1 + j));
	// 		destStateDescription.trim();
	// 		destState = CString(destStateDescription.substr(0, destStateDescription.find_first_of(" ")));

	// 		std::string edgeDescription = line;
	// 		edge = CString(edgeDescription.substr(edgeDescription.find_first_not_of("-")));
	// 		edge = CString(edge.substr(0, edge.find("-")));
	// 		edge.trim();

	// 		// look for the input;output pair on the edge description
	// 		// the first edge description dictates whether we have a normal automaton or an IOAutomaton. (we could also make a command line setting for that but meh..)
	// 		// we have two separators for io edges: ";" and ","
	// 		size_t tempIndex1 = edge.find(";");
	// 		size_t tempIndex2 = edge.find(",");
	// 		std::string delimiter = "";
	// 		if (tempIndex2 != std::string::npos)
	// 			delimiter = ",";
	// 		else if (tempIndex1 != std::string::npos)
	// 			delimiter = ";";

	// 		if (delimiter != "")
	// 		{
	// 			// not sure if we have to split edge label at this stage or later. let's go with later.
	// 			size_t delimiterIndex = edge.find(delimiter);
	// 			inputAction = CString(edge.substr(0, delimiterIndex));
	// 			outputAction = CString(edge.substr(delimiterIndex + 1, edge.length() - delimiterIndex + 1));
	// 			inputAction.trim();
	// 			outputAction.trim();
	// 		}
	// 		else
	// 		{
	// 			outputAction = edge;
	// 		}
	// 		// "" is the standard 'empty' io action here
	// 		if (inputAction == "$")
	// 			inputAction = "";
	// 		if (outputAction == "#")
	// 			outputAction = "";

	// 		IOAEdgeLabel* edgeLabel = new IOAEdgeLabel(inputAction, outputAction);

	// 		// look if the state already exists
	// 		bool duplicateState = false;
	// 		IOAState* q1 = nullptr;
	// 		for (unsigned int i = 0; i < stateMap.size(); i++)
	// 		{
	// 			if (stateMap[i] == srcState)
	// 			{
	// 				duplicateState = true;
	// 				q1 = &(ioa->getStateLabeled(i));
	// 				break;
	// 			}
	// 		}

	// 		if (!duplicateState)
	// 		{
	// 			q1 = ioa->addState(stateId);
	// 			stateMap[stateId] = srcState;
	// 			stateId++;
	// 		}
	// 		// look for state annotations
	// 		srcStateDescription.erase(0, srcState.length());
	// 		srcStateDescription.trim();
	// 		if (j == 0)
	// 			ioa->addInitialState(*q1);

	// 		duplicateState = false;
	// 		IOAState* q2 = nullptr;
	// 		for (unsigned int i = 0; i < stateMap.size(); i++)
	// 		{
	// 			if (stateMap[i] == destState)
	// 			{
	// 				duplicateState = true;

	// 				q2 = &(ioa->getStateLabeled(i));
	// 				break;
	// 			}
	// 		}

	// 		if (!duplicateState)
	// 		{
	// 			q2 = ioa->addState(stateId);
	// 			stateMap[stateId] = destState;
	// 			stateId++;
	// 		}
	// 		// look for state annotations
	// 		destStateDescription.erase(0, destState.length());
	// 		destStateDescription.trim();
	// 		if (destStateDescription.find("i") != std::string::npos || destStateDescription.find("initial") != std::string::npos)
	// 			ioa->addInitialState(*q2);
	// 		if (destStateDescription.find("f") != std::string::npos || destStateDescription.find("final") != std::string::npos)
	// 			ioa->addFinalState(*q2);
	// 		lastState = q2;
	// 		//std::cout << "Edge: -" << inputAction << "," << outputAction << std::endl;
	// 		ioa->addEdge(*q1, *edgeLabel, *q2);
	// 		j++;
	// 	}
	// 	ioa->addFinalState(*lastState);
	// 	std::cout << std::endl
	// 		<< "IO Automaton states size: " << ioa->getStates().size() << std::endl;
	// 	std::cout << "IO Automaton transitions size: " << ioa->getEdges().size() << std::endl;
	// }

	// void SMPLSwithEvents::loadIOAutomatonFromIOAFile(CString fileName)
	// {
	// 	std::cout << "Loading I/O automaton." << std::endl;
	// 	// remember which state maps to which new state
	// 	std::map<CId, CString> stateMap;
	// 	std::ifstream input_stream(fileName);
	// 	if (!input_stream)
	// 		std::cerr << "Can't open input file : " << fileName << std::endl;
	// 	std::string line;
	// 	int lineNumber = 0;
	// 	CId stateId = 0;
	// 	// extract all the text from the input file
	// 	while (getline(input_stream, line))
	// 	{
	// 		lineNumber++;
	// 		size_t i1 = line.find("-");
	// 		size_t i2 = line.find("->");

	// 		if (i1 != std::string::npos && i2 != std::string::npos)
	// 		{
	// 			if (i1 < i2) // if we have a valid line as in "state i f --edge--> state2 f"
	// 			{
	// 				CString srcState = "";
	// 				CString edge = "";
	// 				CString destState = "";
	// 				CString inputAction = "";
	// 				CString outputAction = "";

	// 				CString srcStateDescription = CString(line.substr(0, i1));
	// 				srcStateDescription.trim();
	// 				srcState = CString(srcStateDescription.substr(0, srcStateDescription.find(" ")));

	// 				CString destStateDescription = CString(line.substr(i2 + 2));
	// 				destStateDescription.trim();
	// 				destState = CString(destStateDescription.substr(0, destStateDescription.find_first_of(" ")));

	// 				CString edgeDescription = CString(line.substr(i1, i2 - i1));
	// 				edge = CString(edgeDescription.substr(edgeDescription.find_first_not_of("-")));
	// 				edge = CString(edge.substr(0, edge.find("-")));
	// 				edge.trim();

	// 				// look for the input;output pair on the edge description
	// 				// the first edge description dictates whether we have a normal automaton or an IOAutomaton. (we could also make a command line setting for that but meh..)
	// 				// we have two separators for io edges: ";" and ","
	// 				size_t tempIndex1 = edge.find(";");
	// 				size_t tempIndex2 = edge.find(",");
	// 				std::string delimiter = "";
	// 				if (tempIndex2 != std::string::npos)
	// 					delimiter = ",";
	// 				else if (tempIndex1 != std::string::npos)
	// 					delimiter = ";";
	// 				else
	// 					throw CException("Can not split edge into input and output action. Edge description is invalid. at line: " + CString(lineNumber));

	// 				// not sure if we have to split edge label at this stage or later. let's go with later.
	// 				size_t delimiterIndex = edge.find(delimiter);
	// 				inputAction = CString(edge.substr(0, delimiterIndex));
	// 				outputAction = CString(edge.substr(delimiterIndex + 1, edge.length() - delimiterIndex + 1));
	// 				inputAction.trim();
	// 				outputAction.trim();

	// 				// "" is the standard 'empty' io action here
	// 				if (inputAction == "$")
	// 					inputAction = "";
	// 				if (outputAction == "#")
	// 					outputAction = "";

	// 				IOAEdgeLabel* edgeLabel = new IOAEdgeLabel(inputAction, outputAction);

	// 				// look if the state already exists
	// 				bool duplicateState = false;
	// 				IOAState* q1 = nullptr;
	// 				for (unsigned int i = 0; i < stateMap.size(); i++)
	// 				{
	// 					if (stateMap[i] == srcState)
	// 					{
	// 						duplicateState = true;
	// 						q1 = &(ioa->getStateLabeled(i));
	// 						break;
	// 					}
	// 				}

	// 				if (!duplicateState)
	// 				{
	// 					q1 = ioa->addState(stateId);
	// 					stateMap[stateId] = srcState;
	// 					stateId++;
	// 				}
	// 				// look for state annotations
	// 				srcStateDescription.erase(0, srcState.length());
	// 				srcStateDescription.trim();
	// 				if (srcStateDescription.find("i") != std::string::npos || srcStateDescription.find("initial") != std::string::npos)
	// 					ioa->addInitialState(*q1);
	// 				if (srcStateDescription.find("f") != std::string::npos || srcStateDescription.find("final") != std::string::npos)
	// 					ioa->addFinalState(*q1);

	// 				duplicateState = false;
	// 				IOAState* q2 = nullptr;
	// 				for (unsigned int i = 0; i < stateMap.size(); i++)
	// 				{
	// 					if (stateMap[i] == destState)
	// 					{
	// 						duplicateState = true;

	// 						q2 = &(ioa->getStateLabeled(i));
	// 						break;
	// 					}
	// 				}

	// 				if (!duplicateState)
	// 				{
	// 					q2 = ioa->addState(stateId);
	// 					stateMap[stateId] = destState;
	// 					stateId++;
	// 				}
	// 				// look for state annotations
	// 				destStateDescription.erase(0, destState.length());
	// 				destStateDescription.trim();
	// 				if (destStateDescription.find("i") != std::string::npos || destStateDescription.find("initial") != std::string::npos)
	// 					ioa->addInitialState(*q2);
	// 				if (destStateDescription.find("f") != std::string::npos || destStateDescription.find("final") != std::string::npos)
	// 					ioa->addFinalState(*q2);

	// 				ioa->addEdge(*q1, *edgeLabel, *q2);
	// 			}
	// 			else
	// 			{
	// 				throw CException("invalid line in ioa file! at line: " + CString(lineNumber));
	// 			}
	// 		}
	// 	}
	// }

	// void SMPLS::loadMPMatricesFromMPTFile(CString file)
	// {

	// 	std::cout << "Loading max-plus matrices." << std::endl;
	// 	// Open file
	// 	CDoc* appGraphDoc = CParseFile(file);
	// 	sm = new ScenarioMatrices();

	// 	// Locate the root element
	// 	CNode* rootNode = CGetRootNode(appGraphDoc);

	// 	// TODO : properly check if this is a valid mpt
	// 	/*if (rootNode->name != temp)
	// 	throw CException("Root element in file '" + file + "' is not "
	// 		"mpt:MaxPlusSpecification");*/

	// 	CNode* matrices = CGetChildNode(rootNode, "matrices");

	// 	// go through every matrix in the xml
	// 	// and create their Matrix object
	// 	while (matrices != nullptr)
	// 	{
	// 		// get number of cols and rows to initialize the matrix
	// 		int rows = CGetNumberOfChildNodes(matrices, "rows");
	// 		int cols = 0;
	// 		CNode* rowNode = CGetChildNode(matrices, "rows");
	// 		if (rowNode != nullptr)
	// 			cols = CGetNumberOfChildNodes(rowNode, "values");

	// 		// init the matrix
	// 		std::shared_ptr<Matrix> matrix = new Matrix(rows, cols);

	// 		// reset the ints to be used as indices afterward
	// 		rows = 0;
	// 		cols = 0;

	// 		while (rowNode != nullptr)
	// 		{
	// 			cols = 0;
	// 			CNode* vNode = CGetChildNode(rowNode, "values");
	// 			while (vNode != nullptr)
	// 			{
	// 				std::string content = CGetNodeContent(vNode);

	// 				if (content != "-Infinity")
	// 				{
	// 					double value = atof(content.c_str());
	// 					matrix->put(rows, cols, value);
	// 				}
	// 				cols++;
	// 				vNode = CNextNode(vNode, "values");
	// 			}
	// 			rows++;
	// 			rowNode = CNextNode(rowNode, "rows");
	// 		}

	// 		// we transpose the matrix, this is needed when reading from LSAT mpt files.
	// 		// Gaubert MPA uses transposed version
	// 		//matrix = matrix->getTransposedCopy();
	// 		CString matrixName = CGetAttribute(matrices, "name");
	// 		(*sm)[matrixName] = matrix;
	// 		matrices = CNextNode(matrices, "matrices");
	// 	}
	// }

	// void SMPLSwithEvents::loadActivities(CString file)
	// {
	// 	std::cout << "Loading activities." << std::endl;
	// 	// remember which state maps to which new state
	// 	std::ifstream input_stream(file);
	// 	if (!input_stream)
	// 		std::cerr << "Can't open input file : " << file << std::endl;
	// 	std::string line;

	// 	CString lastActivity = "";
	// 	// extract all the text from the input file line by line
	// 	while (getline(input_stream, line))
	// 	{

	// 		size_t i = line.find("activity");
	// 		if (i != std::string::npos)
	// 		{
	// 			// get the activity name
	// 			lastActivity = line.substr(i + 9);
	// 			lastActivity.trim();
	// 			lastActivity = lastActivity.erase(lastActivity.find_first_of("{"));
	// 			lastActivity.trim();
	// 			std::string activityString = loadEntity(input_stream, line);

	// 			i = activityString.find("events");
	// 			if (i != std::string::npos)
	// 			{
	// 				std::string eventString = activityString.substr(i + 6);
	// 				i = eventString.find("{");
	// 				eventString = eventString.substr(i + 1, eventString.find("}") - i - 1);
	// 				eventString.erase(std::remove(eventString.begin(), eventString.end(), '\t'), eventString.end()); // lsat fills a lot of \t in files which make their way into here, we get rid of them

	// 				std::stringstream ss(eventString);
	// 				Event str;
	// 				while (getline(ss, str, ','))
	// 				{
	// 					str.trim();
	// 					// the magic we have been waiting for happens here.
	// 					// add to sigma
	// 					sigma->insert(sigma->end(), make_pair(lastActivity, str));
	// 				}
	// 			}
	// 		}

	// 		i = line.find("event");
	// 		if (i != std::string::npos)
	// 		{
	// 			// get the event name
	// 			Event eventName = line.substr(i + 6);
	// 			eventName.trim();
	// 			eventName = eventName.erase(eventName.find_first_of(" "));

	// 			eventName.trim();
	// 			std::string eventString = loadEntity(input_stream, line);
	// 			//
	// 			i = eventString.find("{");
	// 			eventString = eventString.substr(i + 1, eventString.find("}") - i - 1);
	// 			eventString.erase(std::remove(eventString.begin(), eventString.end(), '\t'), eventString.end()); // lsat fills a lot of \t in files which make their way into here, we get rid of them

	// 			// go to outcomes
	// 			eventString = eventString.substr(eventString.find("outcomes") + 8);
	// 			eventString = eventString.substr(eventString.find(":") + 1);

	// 			std::stringstream ss(eventString);
	// 			EventOutcome str;
	// 			while (getline(ss, str, ','))
	// 			{
	// 				str.trim();
	// 				// the magic we have been waiting for happens here.
	// 				// add to gamma
	// 				gamma->insert(gamma->end(), make_pair(eventName, str));
	// 			}
	// 		}
	// 	}
	// }
}  // namespace MaxPlus
