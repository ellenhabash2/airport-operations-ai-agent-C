import {
    Plane,
    DoorOpen,
    PlaneTakeoff,
    CloudSun,
} from "lucide-react";
import PropTypes from "prop-types";

const questions = [
    {
        id: 1,
        icon: Plane,
        title: "What is the status of runway 27?",
        subtitle: "Check runway operational status",
    },
    {
        id: 2,
        icon: DoorOpen,
        title: "Which gates are available in Terminal 1?",
        subtitle: "View gate availability",
    },
    {
        id: 3,
        icon: PlaneTakeoff,
        title: "Show me all delayed flights",
        subtitle: "View delayed flights",
    },
    {
        id: 4,
        icon: CloudSun,
        title: "What is the current weather at the airport?",
        subtitle: "Get current weather report",
    },
];

export default function SuggestedQuestions({onQuestionClick,}) {

    return (

        <aside className="suggested-questions">

            <h3>Suggested Questions</h3>

            <p className="suggested-subtitle">
                Click a question to ask AeroMind
            </p>

            <div className="questions-list">

                {questions.map((question) => {

                    const Icon = question.icon;

                    return (

                        <button
                            key={question.id}
                            className="question-card"
                            onClick={() => onQuestionClick(question.title)}
                        >

                            <div className="question-icon">

                                <Icon size={22} />

                            </div>

                            <div className="question-text">

                                <h4>{question.title}</h4>

                                <span>{question.subtitle}</span>

                            </div>

                        </button>

                    );

                })}

            </div>

        </aside>

    );

}

SuggestedQuestions.propTypes = {
    onQuestionClick: PropTypes.func.isRequired,
};
